#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#endif

namespace arcllm::research::lmax_p1 {

struct RingCounters {
    std::uint64_t publish_count = 0;
    std::uint64_t consume_count = 0;
    std::uint64_t max_occupancy = 0;
    std::uint64_t full_backpressure_observations = 0;
    std::uint64_t slot_spin_count = 0;
    std::uint64_t slot_switch_to_thread_count = 0;
    std::uint64_t slot_wait_on_address_count = 0;
    std::uint64_t slot_wake_count = 0;
    std::uint64_t sequence_gap_count = 0;
};

template <typename T, std::size_t Capacity>
class SequencedRing {
    static_assert(Capacity >= 2, "ring capacity must be >= 2");
    static_assert((Capacity & (Capacity - 1)) == 0, "ring capacity must be a power of two");
    static_assert(std::is_copy_assignable<T>::value, "ring payload must be copy assignable");

    struct alignas(64) Slot {
        std::atomic<std::uint64_t> sequence{0};
        T value{};
    };

public:
    SequencedRing() {
        for (std::size_t i = 0; i < Capacity; ++i) {
            slots_[i].sequence.store(static_cast<std::uint64_t>(i), std::memory_order_relaxed);
        }
    }

    SequencedRing(const SequencedRing&) = delete;
    SequencedRing& operator=(const SequencedRing&) = delete;

    void publish(const T& value) {
        publish_prepare(value, [](T&) noexcept {});
    }

    template <typename Prepare>
    void publish_prepare(const T& source, Prepare&& prepare) {
        const std::uint64_t seq = producer_sequence_;
        Slot& slot = slots_[static_cast<std::size_t>(seq) & (Capacity - 1)];
        wait_until(slot, seq, true);

        slot.value = source;
        std::forward<Prepare>(prepare)(slot.value);
        slot.sequence.store(seq + 1u, std::memory_order_release);
        wake(slot.sequence);
        producer_sequence_ = seq + 1u;

        const std::uint64_t published =
            publish_count_.fetch_add(1u, std::memory_order_relaxed) + 1u;
        const std::uint64_t consumed = consume_count_.load(std::memory_order_relaxed);
        update_max_occupancy(published - consumed);
    }

    T consume() {
        return consume_prepare([](const T&) noexcept {});
    }

    template <typename ConsumePrepare>
    T consume_prepare(ConsumePrepare&& prepare) {
        const std::uint64_t seq = consumer_sequence_;
        Slot& slot = slots_[static_cast<std::size_t>(seq) & (Capacity - 1)];
        wait_until(slot, seq + 1u, false);

        T value = slot.value;
        std::forward<ConsumePrepare>(prepare)(value);
        slot.sequence.store(seq + static_cast<std::uint64_t>(Capacity), std::memory_order_release);
        wake(slot.sequence);
        consumer_sequence_ = seq + 1u;
        consume_count_.fetch_add(1u, std::memory_order_relaxed);
        return value;
    }

    void record_sequence_gap() noexcept {
        sequence_gap_count_.fetch_add(1u, std::memory_order_relaxed);
    }

    RingCounters counters() const noexcept {
        RingCounters out;
        out.publish_count = publish_count_.load(std::memory_order_relaxed);
        out.consume_count = consume_count_.load(std::memory_order_relaxed);
        out.max_occupancy = max_occupancy_.load(std::memory_order_relaxed);
        out.full_backpressure_observations =
            full_backpressure_observations_.load(std::memory_order_relaxed);
        out.slot_spin_count = slot_spin_count_.load(std::memory_order_relaxed);
        out.slot_switch_to_thread_count =
            slot_switch_to_thread_count_.load(std::memory_order_relaxed);
        out.slot_wait_on_address_count =
            slot_wait_on_address_count_.load(std::memory_order_relaxed);
        out.slot_wake_count = slot_wake_count_.load(std::memory_order_relaxed);
        out.sequence_gap_count = sequence_gap_count_.load(std::memory_order_relaxed);
        return out;
    }

    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    void wait_until(
        Slot& slot,
        const std::uint64_t target,
        const bool producer_wait) noexcept {

        bool counted_backpressure = false;
        std::uint32_t active_spins = 0;

        for (;;) {
            const std::uint64_t current =
                slot.sequence.load(std::memory_order_acquire);
            if (current == target) {
                return;
            }

            if (producer_wait && !counted_backpressure) {
                full_backpressure_observations_.fetch_add(1u, std::memory_order_relaxed);
                counted_backpressure = true;
            }

            if (active_spins < 16u) {
                ++active_spins;
                slot_spin_count_.fetch_add(1u, std::memory_order_relaxed);
#ifdef _WIN32
                YieldProcessor();
#else
                std::this_thread::yield();
#endif
                continue;
            }

            slot_switch_to_thread_count_.fetch_add(1u, std::memory_order_relaxed);
#ifdef _WIN32
            SwitchToThread();
#else
            std::this_thread::yield();
#endif
            if (slot.sequence.load(std::memory_order_acquire) == target) {
                return;
            }

#ifdef _WIN32
            std::uint64_t expected =
                slot.sequence.load(std::memory_order_relaxed);
            slot_wait_on_address_count_.fetch_add(1u, std::memory_order_relaxed);
            WaitOnAddress(
                reinterpret_cast<volatile VOID*>(&slot.sequence),
                &expected,
                sizeof(expected),
                INFINITE);
#else
            std::this_thread::yield();
#endif
            active_spins = 0;
        }
    }

    void wake(std::atomic<std::uint64_t>& sequence) noexcept {
#ifdef _WIN32
        WakeByAddressSingle(reinterpret_cast<PVOID>(&sequence));
#endif
        slot_wake_count_.fetch_add(1u, std::memory_order_relaxed);
    }

    void update_max_occupancy(std::uint64_t value) noexcept {
        std::uint64_t current = max_occupancy_.load(std::memory_order_relaxed);
        while (current < value &&
               !max_occupancy_.compare_exchange_weak(
                   current, value,
                   std::memory_order_relaxed,
                   std::memory_order_relaxed)) {
        }
    }

    std::array<Slot, Capacity> slots_{};
    alignas(64) std::uint64_t producer_sequence_ = 0;
    alignas(64) std::uint64_t consumer_sequence_ = 0;

    alignas(64) std::atomic<std::uint64_t> publish_count_{0};
    std::atomic<std::uint64_t> consume_count_{0};
    std::atomic<std::uint64_t> max_occupancy_{0};
    std::atomic<std::uint64_t> full_backpressure_observations_{0};
    std::atomic<std::uint64_t> slot_spin_count_{0};
    std::atomic<std::uint64_t> slot_switch_to_thread_count_{0};
    std::atomic<std::uint64_t> slot_wait_on_address_count_{0};
    std::atomic<std::uint64_t> slot_wake_count_{0};
    std::atomic<std::uint64_t> sequence_gap_count_{0};
};

} // namespace arcllm::research::lmax_p1
