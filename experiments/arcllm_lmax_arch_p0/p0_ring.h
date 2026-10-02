#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <type_traits>

namespace arcllm::research::lmax_p0 {

struct RingCounters {
    std::uint64_t publish_count = 0;
    std::uint64_t consume_count = 0;
    std::uint64_t max_occupancy = 0;
    std::uint64_t full_ring_observations = 0;
    std::uint64_t spin_iterations = 0;
    std::uint64_t yield_count = 0;
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
        const std::uint64_t seq = producer_sequence_;
        Slot& slot = slots_[static_cast<std::size_t>(seq) & (Capacity - 1)];
        bool observed_full = false;
        std::uint32_t local_spins = 0;

        while (slot.sequence.load(std::memory_order_acquire) != seq) {
            if (!observed_full) {
                full_ring_observations_.fetch_add(1, std::memory_order_relaxed);
                observed_full = true;
            }
            spin_iterations_.fetch_add(1, std::memory_order_relaxed);
            if (++local_spins == 128u) {
                local_spins = 0;
                yield_count_.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::yield();
            }
        }

        slot.value = value;
        slot.sequence.store(seq + 1u, std::memory_order_release);
        producer_sequence_ = seq + 1u;

        const std::uint64_t published = publish_count_.fetch_add(1, std::memory_order_relaxed) + 1u;
        const std::uint64_t consumed = consume_count_.load(std::memory_order_relaxed);
        update_max_occupancy(published - consumed);
    }

    T consume() {
        const std::uint64_t seq = consumer_sequence_;
        Slot& slot = slots_[static_cast<std::size_t>(seq) & (Capacity - 1)];
        std::uint32_t local_spins = 0;

        while (slot.sequence.load(std::memory_order_acquire) != seq + 1u) {
            spin_iterations_.fetch_add(1, std::memory_order_relaxed);
            if (++local_spins == 128u) {
                local_spins = 0;
                yield_count_.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::yield();
            }
        }

        T value = slot.value;
        slot.sequence.store(seq + static_cast<std::uint64_t>(Capacity), std::memory_order_release);
        consumer_sequence_ = seq + 1u;
        consume_count_.fetch_add(1, std::memory_order_relaxed);
        return value;
    }

    void record_sequence_gap() noexcept {
        sequence_gap_count_.fetch_add(1, std::memory_order_relaxed);
    }

    RingCounters counters() const noexcept {
        RingCounters out;
        out.publish_count = publish_count_.load(std::memory_order_relaxed);
        out.consume_count = consume_count_.load(std::memory_order_relaxed);
        out.max_occupancy = max_occupancy_.load(std::memory_order_relaxed);
        out.full_ring_observations = full_ring_observations_.load(std::memory_order_relaxed);
        out.spin_iterations = spin_iterations_.load(std::memory_order_relaxed);
        out.yield_count = yield_count_.load(std::memory_order_relaxed);
        out.sequence_gap_count = sequence_gap_count_.load(std::memory_order_relaxed);
        return out;
    }

    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    void update_max_occupancy(std::uint64_t value) noexcept {
        std::uint64_t current = max_occupancy_.load(std::memory_order_relaxed);
        while (current < value &&
               !max_occupancy_.compare_exchange_weak(
                   current, value, std::memory_order_relaxed, std::memory_order_relaxed)) {
        }
    }

    std::array<Slot, Capacity> slots_{};
    alignas(64) std::uint64_t producer_sequence_ = 0;
    alignas(64) std::uint64_t consumer_sequence_ = 0;

    alignas(64) std::atomic<std::uint64_t> publish_count_{0};
    std::atomic<std::uint64_t> consume_count_{0};
    std::atomic<std::uint64_t> max_occupancy_{0};
    std::atomic<std::uint64_t> full_ring_observations_{0};
    std::atomic<std::uint64_t> spin_iterations_{0};
    std::atomic<std::uint64_t> yield_count_{0};
    std::atomic<std::uint64_t> sequence_gap_count_{0};
};

} // namespace arcllm::research::lmax_p0
