#include "p1_ring.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <malloc.h>
#endif

namespace {

std::atomic<bool> g_count_allocations{false};
std::atomic<std::uint64_t> g_allocation_count{0};

void note_allocation() noexcept {
    if (g_count_allocations.load(std::memory_order_relaxed)) {
        g_allocation_count.fetch_add(1u, std::memory_order_relaxed);
    }
}

std::uint64_t now_ns() noexcept {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

void monotonic_busy_delay(
    const std::uint64_t consume_timestamp_ns,
    const std::uint64_t service_delay_ns,
    std::uint64_t* audit_iterations) noexcept {

    if (service_delay_ns == 0u) return;
    const std::uint64_t deadline_ns = consume_timestamp_ns + service_delay_ns;
    std::uint64_t iterations = 0u;
    while (now_ns() < deadline_ns) {
        ++iterations;
#ifdef _WIN32
        YieldProcessor();
#else
        std::this_thread::yield();
#endif
    }
    if (audit_iterations) *audit_iterations += iterations;
}

std::string read_text_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open execution authorization");
    return std::string(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());
}

bool execution_authorized(const std::string& path) {
    if (path.empty()) return false;
    const std::string text = read_text_file(path);
    std::string compact;
    compact.reserve(text.size());
    for (char ch : text) {
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') compact.push_back(ch);
    }
    return compact.find(
        "\"decision\":\"ARCLLM_LMAX_ARCH_P1_E_EXECUTION_AUTHORIZED\"") != std::string::npos;
}

struct Event {
    std::uint64_t sequence_id = 0u;
    std::uint64_t publish_timestamp_ns = 0u;
};

struct ControlCounters {
    std::uint64_t max_occupancy = 0u;
    std::uint64_t full_backpressure_observations = 0u;
    std::uint64_t producer_condition_variable_wait_count = 0u;
    std::uint64_t consumer_condition_variable_wait_count = 0u;
    std::uint64_t spin_count = 0u;
    std::uint64_t switch_to_thread_count = 0u;
    std::uint64_t wait_on_address_count = 0u;
    std::uint64_t wake_count = 0u;
    std::uint64_t service_delay_loop_iterations = 0u;
    std::int64_t first_ordering_mismatch = -1;
};

template <std::size_t Capacity>
class DirectLockedSpsc {
public:
    static_assert(Capacity == 64u, "DIRECT_LOCKED_SPSC64 capacity is frozen at 64");

    template <typename Prepare>
    void publish_prepare(const Event& source, Prepare&& prepare) {
        std::unique_lock<std::mutex> lock(mutex_);
        bool counted = false;
        while (count_ == Capacity) {
            if (!counted) {
                ++counters_.full_backpressure_observations;
                counted = true;
            }
            ++counters_.producer_condition_variable_wait_count;
            not_full_.wait(lock, [&] { return count_ < Capacity; });
        }

        slots_[tail_] = source;
        const std::size_t published_index = tail_;
        tail_ = (tail_ + 1u) & (Capacity - 1u);
        std::forward<Prepare>(prepare)(slots_[published_index]);
        ++count_;
        if (count_ > counters_.max_occupancy) counters_.max_occupancy = count_;
        ++counters_.wake_count;
        lock.unlock();
        not_empty_.notify_one();
    }

    Event consume() {
        return consume_prepare([](const Event&) noexcept {});
    }

    template <typename ConsumePrepare>
    Event consume_prepare(ConsumePrepare&& prepare) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (count_ == 0u) {
            ++counters_.consumer_condition_variable_wait_count;
            not_empty_.wait(lock, [&] { return count_ > 0u; });
        }
        Event value = slots_[head_];
        head_ = (head_ + 1u) & (Capacity - 1u);
        --count_;
        std::forward<ConsumePrepare>(prepare)(value);
        ++counters_.wake_count;
        lock.unlock();
        not_full_.notify_one();
        return value;
    }

    ControlCounters counters() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return counters_;
    }

private:
    std::array<Event, Capacity> slots_{};
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::size_t head_ = 0u;
    std::size_t tail_ = 0u;
    std::size_t count_ = 0u;
    ControlCounters counters_{};
};

void write_u64le(
    const std::string& path,
    const std::vector<std::uint64_t>& values) {

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot open raw binary output: " + path);
    for (std::uint64_t value : values) {
        const unsigned char bytes[8] = {
            static_cast<unsigned char>((value >> 0u) & 0xffu),
            static_cast<unsigned char>((value >> 8u) & 0xffu),
            static_cast<unsigned char>((value >> 16u) & 0xffu),
            static_cast<unsigned char>((value >> 24u) & 0xffu),
            static_cast<unsigned char>((value >> 32u) & 0xffu),
            static_cast<unsigned char>((value >> 40u) & 0xffu),
            static_cast<unsigned char>((value >> 48u) & 0xffu),
            static_cast<unsigned char>((value >> 56u) & 0xffu)
        };
        out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    }
}

struct RunOutput {
    std::uint64_t run_start_ns = 0u;
    std::uint64_t run_end_ns = 0u;
    std::uint64_t allocation_counter_start = 0u;
    std::uint64_t allocation_counter_end = 0u;
    ControlCounters counters{};
};

RunOutput run_direct(
    const std::uint64_t events,
    const std::uint64_t service_delay_ns,
    std::vector<std::uint64_t>& consumed,
    std::vector<std::uint64_t>& latency) {

    DirectLockedSpsc<64> queue;
    RunOutput out;
    std::atomic<bool> start{false};
    std::uint64_t delay_iterations = 0u;
    std::int64_t first_mismatch = -1;

    std::thread producer([&] {
        while (!start.load(std::memory_order_acquire)) {
#ifdef _WIN32
            YieldProcessor();
#else
            std::this_thread::yield();
#endif
        }
        for (std::uint64_t i = 0; i < events; ++i) {
            Event e{i, 0u};
            queue.publish_prepare(e, [&](Event& slot_value) {
                slot_value.publish_timestamp_ns = now_ns();
            });
        }
    });

    std::thread consumer([&] {
        while (!start.load(std::memory_order_acquire)) {
#ifdef _WIN32
            YieldProcessor();
#else
            std::this_thread::yield();
#endif
        }
        for (std::uint64_t i = 0; i < events; ++i) {
            std::uint64_t consume_timestamp_ns = 0u;
            const Event e = queue.consume_prepare([&](const Event&) {
                consume_timestamp_ns = now_ns();
            });
            consumed[static_cast<std::size_t>(i)] = e.sequence_id;
            latency[static_cast<std::size_t>(i)] =
                consume_timestamp_ns - e.publish_timestamp_ns;
            if (first_mismatch < 0 && e.sequence_id != i) {
                first_mismatch = static_cast<std::int64_t>(i);
            }
            monotonic_busy_delay(
                consume_timestamp_ns,
                service_delay_ns,
                &delay_iterations);
        }
    });

    g_allocation_count.store(0u, std::memory_order_relaxed);
    out.allocation_counter_start =
        g_allocation_count.load(std::memory_order_relaxed);
    g_count_allocations.store(true, std::memory_order_release);
    out.run_start_ns = now_ns();
    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    out.run_end_ns = now_ns();
    out.allocation_counter_end =
        g_allocation_count.load(std::memory_order_relaxed);
    g_count_allocations.store(false, std::memory_order_release);

    out.counters = queue.counters();
    out.counters.service_delay_loop_iterations = delay_iterations;
    out.counters.first_ordering_mismatch = first_mismatch;
    return out;
}

RunOutput run_ring(
    const std::uint64_t events,
    const std::uint64_t service_delay_ns,
    std::vector<std::uint64_t>& consumed,
    std::vector<std::uint64_t>& latency) {

    using Ring = arcllm::research::lmax_p1::SequencedRing<Event, 64>;
    Ring ring;
    RunOutput out;
    std::atomic<bool> start{false};
    std::uint64_t delay_iterations = 0u;
    std::int64_t first_mismatch = -1;

    std::thread producer([&] {
        while (!start.load(std::memory_order_acquire)) {
#ifdef _WIN32
            YieldProcessor();
#else
            std::this_thread::yield();
#endif
        }
        for (std::uint64_t i = 0; i < events; ++i) {
            Event e{i, 0u};
            ring.publish_prepare(e, [&](Event& slot_value) {
                slot_value.publish_timestamp_ns = now_ns();
            });
        }
    });

    std::thread consumer([&] {
        while (!start.load(std::memory_order_acquire)) {
#ifdef _WIN32
            YieldProcessor();
#else
            std::this_thread::yield();
#endif
        }
        for (std::uint64_t i = 0; i < events; ++i) {
            std::uint64_t consume_timestamp_ns = 0u;
            const Event e = ring.consume_prepare([&](const Event&) {
                consume_timestamp_ns = now_ns();
            });
            consumed[static_cast<std::size_t>(i)] = e.sequence_id;
            latency[static_cast<std::size_t>(i)] =
                consume_timestamp_ns - e.publish_timestamp_ns;
            if (first_mismatch < 0 && e.sequence_id != i) {
                first_mismatch = static_cast<std::int64_t>(i);
                ring.record_sequence_gap();
            }
            monotonic_busy_delay(
                consume_timestamp_ns,
                service_delay_ns,
                &delay_iterations);
        }
    });

    g_allocation_count.store(0u, std::memory_order_relaxed);
    out.allocation_counter_start =
        g_allocation_count.load(std::memory_order_relaxed);
    g_count_allocations.store(true, std::memory_order_release);
    out.run_start_ns = now_ns();
    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    out.run_end_ns = now_ns();
    out.allocation_counter_end =
        g_allocation_count.load(std::memory_order_relaxed);
    g_count_allocations.store(false, std::memory_order_release);

    const auto rc = ring.counters();
    out.counters.max_occupancy = rc.max_occupancy;
    out.counters.full_backpressure_observations =
        rc.full_backpressure_observations;
    out.counters.spin_count = rc.slot_spin_count;
    out.counters.switch_to_thread_count = rc.slot_switch_to_thread_count;
    out.counters.wait_on_address_count = rc.slot_wait_on_address_count;
    out.counters.wake_count = rc.slot_wake_count;
    out.counters.service_delay_loop_iterations = delay_iterations;
    out.counters.first_ordering_mismatch = first_mismatch;
    return out;
}

void write_manifest(
    const std::string& path,
    const std::string& arm,
    const std::uint64_t delay_ns,
    const std::uint64_t events,
    const RunOutput& r,
    const std::string& sequence_path,
    const std::string& latency_path) {

    const std::uint64_t allocs =
        r.allocation_counter_end >= r.allocation_counter_start
            ? r.allocation_counter_end - r.allocation_counter_start : 0u;
    const double backpressure_rate =
        events ? static_cast<double>(r.counters.full_backpressure_observations) /
                     static_cast<double>(events) : 0.0;

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot open manifest output");
    out << std::setprecision(17);
    out << "{\n"
        << "  \"schema\":\"arcllm.lmax_arch_p1.control_path_observation.v0.4\",\n"
        << "  \"arm\":\"" << arm << "\",\n"
        << "  \"service_delay_ns\":" << delay_ns << ",\n"
        << "  \"service_delay_mechanism\":\"MONOTONIC_BUSY_DELAY\",\n"
        << "  \"run_start_ns\":" << r.run_start_ns << ",\n"
        << "  \"run_end_ns\":" << r.run_end_ns << ",\n"
        << "  \"event_count\":" << events << ",\n"
        << "  \"allocation_counter_start\":" << r.allocation_counter_start << ",\n"
        << "  \"allocation_counter_end\":" << r.allocation_counter_end << ",\n"
        << "  \"steady_state_allocation_count\":" << allocs << ",\n"
        << "  \"max_occupancy\":" << r.counters.max_occupancy << ",\n"
        << "  \"full_backpressure_observations\":" << r.counters.full_backpressure_observations << ",\n"
        << "  \"full_backpressure_observation_rate\":" << backpressure_rate << ",\n"
        << "  \"spin_count\":" << r.counters.spin_count << ",\n"
        << "  \"switch_to_thread_count\":" << r.counters.switch_to_thread_count << ",\n"
        << "  \"wait_on_address_count\":" << r.counters.wait_on_address_count << ",\n"
        << "  \"producer_condition_variable_wait_count\":" << r.counters.producer_condition_variable_wait_count << ",\n"
        << "  \"consumer_condition_variable_wait_count\":" << r.counters.consumer_condition_variable_wait_count << ",\n"
        << "  \"wake_count\":" << r.counters.wake_count << ",\n"
        << "  \"service_delay_loop_iterations\":" << r.counters.service_delay_loop_iterations << ",\n"
        << "  \"first_ordering_mismatch\":" << r.counters.first_ordering_mismatch << ",\n"
        << "  \"consumed_sequence_ids_path\":\"" << sequence_path << "\",\n"
        << "  \"publish_to_consume_latency_ns_path\":\"" << latency_path << "\"\n"
        << "}\n";
}

} // namespace

void* operator new(std::size_t size) {
    if (void* p = std::malloc(size ? size : 1u)) {
        note_allocation();
        return p;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    if (void* p = std::malloc(size ? size : 1u)) {
        note_allocation();
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
        note_allocation();
        return p;
    }
#else
    void* p = nullptr;
    if (posix_memalign(&p, static_cast<std::size_t>(alignment), size ? size : 1u) == 0) {
        note_allocation();
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

int main(int argc, char** argv) {
    try {
        bool describe = false;
        std::string authorization;
        std::string arm;
        std::string manifest;
        std::string sequence_out;
        std::string latency_out;
        std::uint64_t delay_ns = 0u;
        std::uint64_t events = 1000000u;

        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto need = [&](const char* flag) {
                if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + flag);
                return std::string(argv[++i]);
            };
            if (a == "--describe") describe = true;
            else if (a == "--authorization") authorization = need("--authorization");
            else if (a == "--arm") arm = need("--arm");
            else if (a == "--delay-ns") delay_ns = std::stoull(need("--delay-ns"));
            else if (a == "--events") events = std::stoull(need("--events"));
            else if (a == "--manifest") manifest = need("--manifest");
            else if (a == "--sequence-out") sequence_out = need("--sequence-out");
            else if (a == "--latency-out") latency_out = need("--latency-out");
            else throw std::runtime_error("unknown argument: " + a);
        }

        if (describe) {
            std::cout
                << "ARCLLM_LMAX_ARCH_P1_CONTROL_PATH=BUILDONLY_DESCRIBE\n"
                << "ARMS=DIRECT_LOCKED_SPSC64,LMAX_RING_P1\n"
                << "EVENTS_PER_ARM_PER_DELAY=1000000\n"
                << "DELAYS_NS=0,10000,100000\n"
                << "SERVICE_DELAY=MONOTONIC_BUSY_DELAY\n"
                << "MODEL_EXECUTED=false\n"
                << "VULKAN_INITIALIZED=false\n";
            return 0;
        }

        if (!execution_authorized(authorization)) {
            throw std::runtime_error("outcome execution blocked: missing or invalid P1 E authorization");
        }
        if (events != 1000000u) {
            throw std::runtime_error("P1 E control-path event count must be exactly 1000000");
        }
        if (delay_ns != 0u && delay_ns != 10000u && delay_ns != 100000u) {
            throw std::runtime_error("P1 E delay must be 0, 10000, or 100000 ns");
        }
        if (arm != "direct_locked_spsc64" && arm != "lmax_ring_p1") {
            throw std::runtime_error("P1 E arm must be direct_locked_spsc64 or lmax_ring_p1");
        }
        if (manifest.empty() || sequence_out.empty() || latency_out.empty()) {
            throw std::runtime_error("manifest/sequence/latency output paths are required");
        }

        std::vector<std::uint64_t> consumed(static_cast<std::size_t>(events));
        std::vector<std::uint64_t> latency(static_cast<std::size_t>(events));

        RunOutput result =
            arm == "direct_locked_spsc64"
                ? run_direct(events, delay_ns, consumed, latency)
                : run_ring(events, delay_ns, consumed, latency);

        write_u64le(sequence_out, consumed);
        write_u64le(latency_out, latency);
        write_manifest(
            manifest,
            arm == "direct_locked_spsc64" ? "DIRECT_LOCKED_SPSC64" : "LMAX_RING_P1",
            delay_ns,
            events,
            result,
            sequence_out,
            latency_out);

        return result.counters.first_ordering_mismatch < 0 ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "ARCLLM_LMAX_ARCH_P1_CONTROL_PATH_ERROR=" << e.what() << "\n";
        return 2;
    }
}
