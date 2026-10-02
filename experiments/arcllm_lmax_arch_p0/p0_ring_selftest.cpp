#include "p0_ring.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace {
std::atomic<bool> g_count_allocations{false};
std::atomic<std::uint64_t> g_allocation_count{0};

void record_allocation() noexcept {
    if (g_count_allocations.load(std::memory_order_relaxed)) {
        g_allocation_count.fetch_add(1, std::memory_order_relaxed);
    }
}

struct FixtureEvent {
    std::uint64_t sequence = 0;
};
} // namespace

void* operator new(std::size_t size) {
    if (void* p = std::malloc(size ? size : 1u)) {
        record_allocation();
        return p;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    if (void* p = std::malloc(size ? size : 1u)) {
        record_allocation();
        return p;
    }
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

void* operator new(std::size_t size, std::align_val_t alignment) {
#ifdef _WIN32
    if (void* p = _aligned_malloc(size ? size : 1u, static_cast<std::size_t>(alignment))) {
        record_allocation();
        return p;
    }
#else
    void* p = nullptr;
    if (posix_memalign(&p, static_cast<std::size_t>(alignment), size ? size : 1u) == 0) {
        record_allocation();
        return p;
    }
#endif
    throw std::bad_alloc();
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void operator delete(void* p, std::align_val_t) noexcept {
#ifdef _WIN32
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p, std::align_val_t alignment) noexcept {
    ::operator delete(p, alignment);
}
void operator delete(void* p, std::size_t, std::align_val_t alignment) noexcept {
    ::operator delete(p, alignment);
}
void operator delete[](void* p, std::size_t, std::align_val_t alignment) noexcept {
    ::operator delete(p, alignment);
}

int main() {
    using Ring = arcllm::research::lmax_p0::SequencedRing<FixtureEvent, 64>;
    constexpr std::uint64_t kEvents = 8192;

    Ring ring;
    std::atomic<bool> start{false};
    std::atomic<bool> allow_consume{false};
    std::atomic<bool> producer_done{false};
    std::atomic<bool> consumer_done{false};

    std::thread producer([&] {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (std::uint64_t i = 0; i < kEvents; ++i) {
            ring.publish(FixtureEvent{i});
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        while (!allow_consume.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (std::uint64_t expected = 0; expected < kEvents; ++expected) {
            const FixtureEvent event = ring.consume();
            if (event.sequence != expected) {
                ring.record_sequence_gap();
            }
        }
        consumer_done.store(true, std::memory_order_release);
    });

    g_allocation_count.store(0, std::memory_order_relaxed);
    g_count_allocations.store(true, std::memory_order_release);
    start.store(true, std::memory_order_release);

    while (ring.counters().full_ring_observations == 0u) {
        if (producer_done.load(std::memory_order_acquire)) {
            break;
        }
        std::this_thread::yield();
    }
    allow_consume.store(true, std::memory_order_release);

    while (!producer_done.load(std::memory_order_acquire) ||
           !consumer_done.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    g_count_allocations.store(false, std::memory_order_release);
    producer.join();
    consumer.join();

    const auto counters = ring.counters();
    const auto allocations = g_allocation_count.load(std::memory_order_relaxed);

    const bool pass =
        counters.publish_count == kEvents &&
        counters.consume_count == kEvents &&
        counters.sequence_gap_count == 0u &&
        counters.max_occupancy <= Ring::capacity() &&
        counters.full_ring_observations > 0u &&
        allocations == 0u;

    if (!pass) {
        std::cerr
            << "ARCLLM_LMAX_P0_RING_SELFTEST=FAIL"
            << " publish=" << counters.publish_count
            << " consume=" << counters.consume_count
            << " max_occupancy=" << counters.max_occupancy
            << " full_observations=" << counters.full_ring_observations
            << " sequence_gaps=" << counters.sequence_gap_count
            << " steady_state_allocations=" << allocations
            << "\n";
        return 2;
    }

    std::cout
        << "ARCLLM_LMAX_P0_RING_SELFTEST=PASS"
        << " events=" << kEvents
        << " capacity=" << Ring::capacity()
        << " ordering=PASS"
        << " forced_backpressure=PASS"
        << " steady_state_allocations=0"
        << "\n";
    return 0;
}
