#include "p1_ring.h"
#include "../../include/arcllm/v1/runtime.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <malloc.h>
#endif

extern "C" {
using ArcllmLmaxP1TokenObserver = void(*)(std::uint32_t, std::uint32_t, void*);
void arcllm_lmax_p1_set_token_observer(ArcllmLmaxP1TokenObserver observer, void* user);
}

namespace {
using arcllm::v1::runtime::RunRequest;
using arcllm::v1::runtime::RunResult;

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

#ifdef _WIN32
std::uint64_t filetime_u64(const FILETIME& ft) noexcept {
    ULARGE_INTEGER v{};
    v.LowPart = ft.dwLowDateTime;
    v.HighPart = ft.dwHighDateTime;
    return v.QuadPart;
}

std::uint64_t process_cpu_100ns() {
    FILETIME create{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &create, &exit, &kernel, &user)) {
        throw std::runtime_error("GetProcessTimes failed");
    }
    return filetime_u64(kernel) + filetime_u64(user);
}

std::uint32_t logical_processor_count() noexcept {
    const DWORD n = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    return n ? static_cast<std::uint32_t>(n) : 1u;
}
#else
std::uint64_t process_cpu_100ns() { return 0; }
std::uint32_t logical_processor_count() noexcept { return 1u; }
#endif

struct WaitCountersSnapshot {
    std::uint64_t spin_count = 0;
    std::uint64_t switch_to_thread_count = 0;
    std::uint64_t wait_on_address_count = 0;
    std::uint64_t wake_count = 0;
};

struct WaitCountersAtomic {
    std::atomic<std::uint64_t> spin_count{0};
    std::atomic<std::uint64_t> switch_to_thread_count{0};
    std::atomic<std::uint64_t> wait_on_address_count{0};
    std::atomic<std::uint64_t> wake_count{0};

    WaitCountersSnapshot snapshot() const noexcept {
        return {
            spin_count.load(std::memory_order_relaxed),
            switch_to_thread_count.load(std::memory_order_relaxed),
            wait_on_address_count.load(std::memory_order_relaxed),
            wake_count.load(std::memory_order_relaxed)
        };
    }
};

void wait_until_true(std::atomic<bool>& flag, WaitCountersAtomic& counters) noexcept {
    std::uint32_t active_spins = 0;
    for (;;) {
        if (flag.load(std::memory_order_acquire)) return;
        if (active_spins < 16u) {
            ++active_spins;
            counters.spin_count.fetch_add(1u, std::memory_order_relaxed);
#ifdef _WIN32
            YieldProcessor();
#else
            std::this_thread::yield();
#endif
            continue;
        }
        counters.switch_to_thread_count.fetch_add(1u, std::memory_order_relaxed);
#ifdef _WIN32
        SwitchToThread();
#else
        std::this_thread::yield();
#endif
        if (flag.load(std::memory_order_acquire)) return;
#ifdef _WIN32
        const bool expected = false;
        counters.wait_on_address_count.fetch_add(1u, std::memory_order_relaxed);
        WaitOnAddress(
            reinterpret_cast<volatile VOID*>(&flag),
            &expected,
            sizeof(expected),
            INFINITE);
#else
        std::this_thread::yield();
#endif
        active_spins = 0;
    }
}

void publish_true_and_wake(
    std::atomic<bool>& flag,
    WaitCountersAtomic& counters) noexcept {
    flag.store(true, std::memory_order_release);
#ifdef _WIN32
    WakeByAddressSingle(reinterpret_cast<PVOID>(&flag));
#endif
    counters.wake_count.fetch_add(1u, std::memory_order_relaxed);
}

struct TokenTrace {
    static constexpr std::size_t kMaxTokens = 32;
    std::array<std::uint64_t, kMaxTokens> ready_ns{};
    std::array<std::uint32_t, kMaxTokens> token_ids{};
    std::array<std::uint64_t, kMaxTokens> allocation_count_at_ready{};
    std::uint32_t count = 0;
    bool overflow = false;
};

void token_observer(std::uint32_t index, std::uint32_t token_id, void* user) {
    auto* trace = static_cast<TokenTrace*>(user);
    if (!trace || index >= TokenTrace::kMaxTokens || index != trace->count) {
        if (trace) trace->overflow = true;
        return;
    }
    trace->ready_ns[index] = now_ns();
    trace->token_ids[index] = token_id;
    trace->allocation_count_at_ready[index] =
        g_allocation_count.load(std::memory_order_relaxed);
    ++trace->count;
}

// Both experimental arms converge here. This is the only call site to the
// instrumented copy of the canonical ArcLLM generate() body.
RunResult invoke_canonical_semantics(const RunRequest& request, TokenTrace& trace) {
    arcllm_lmax_p1_set_token_observer(&token_observer, &trace);
    try {
        RunResult result = arcllm::v1::runtime::generate(request);
        arcllm_lmax_p1_set_token_observer(nullptr, nullptr);
        return result;
    } catch (...) {
        arcllm_lmax_p1_set_token_observer(nullptr, nullptr);
        throw;
    }
}

struct RuntimeEvent {
    std::uint64_t sequence = 0;
    const RunRequest* request = nullptr;
    RunResult* result = nullptr;
    TokenTrace* trace = nullptr;
    std::atomic<bool>* done = nullptr;
    char* error = nullptr;
    std::size_t error_capacity = 0;
};

struct Workload {
    const char* id = "";
    std::uint32_t cell_index = 0;
    std::uint32_t prefill_tokens = 0;
    std::uint32_t max_new_tokens = 0;
};

Workload workload_for(const std::string& id) {
    if (id == "W1") return {"W1", 1u, 8u, 8u};
    if (id == "W2") return {"W2", 2u, 8u, 32u};
    if (id == "W3") return {"W3", 3u, 64u, 8u};
    if (id == "W4") return {"W4", 4u, 64u, 32u};
    if (id == "W5") return {"W5", 5u, 256u, 8u};
    if (id == "W6") return {"W6", 6u, 256u, 32u};
    throw std::runtime_error("--cell must be W1..W6");
}

std::vector<std::uint32_t> make_tokens(const Workload& w) {
    std::vector<std::uint32_t> ids;
    ids.reserve(w.prefill_tokens);
    for (std::uint32_t i = 0; i < w.prefill_tokens; ++i) {
        const std::uint64_t value =
            1ull + ((7919ull * i + 104729ull * w.cell_index) % 152063ull);
        ids.push_back(static_cast<std::uint32_t>(value));
    }
    return ids;
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
        if (ch != ' ' && ch != '\\t' && ch != '\\r' && ch != '\\n') {
            compact.push_back(ch);
        }
    }
    return compact.find("\"decision\":\"ARCLLM_LMAX_ARCH_P1_E_EXECUTION_AUTHORIZED\"") !=
           std::string::npos;
}

std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8u);
    for (char c : s) {
        if (c == '\\' || c == '"') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out.push_back(c);
    }
    return out;
}

struct Observation {
    bool success = false;
    std::string error;
    RunResult result{};
    TokenTrace trace{};
    std::uint64_t request_start_ns = 0;
    std::uint64_t request_end_ns = 0;
    std::uint64_t cpu_start_100ns = 0;
    std::uint64_t cpu_end_100ns = 0;
    std::uint64_t request_allocation_counter_start = 0;
    std::uint64_t request_allocation_counter_end = 0;
    std::uint64_t decode_allocation_counter_start = 0;
    std::uint64_t decode_allocation_counter_end = 0;
    bool ring_applicable = false;
    arcllm::research::lmax_p1::RingCounters ring{};
    WaitCountersSnapshot completion_wait{};
    WaitCountersSnapshot consumer_ready_wait{};
};

struct IdentityEvidence {
    std::string runner_sha256;
    std::string model_sha256;
    std::string shader_manifest_sha256;
    std::string authorization_sha256;
    std::string evidence_contract_sha256;
};

Observation run_direct(const RunRequest& request) {
    Observation o;
    g_allocation_count.store(0u, std::memory_order_relaxed);
    o.request_allocation_counter_start =
        g_allocation_count.load(std::memory_order_relaxed);
    g_count_allocations.store(true, std::memory_order_release);
    o.cpu_start_100ns = process_cpu_100ns();
    o.request_start_ns = now_ns();
    try {
        o.result = invoke_canonical_semantics(request, o.trace);
        o.success = true;
    } catch (const std::exception& e) {
        o.error = e.what();
    }
    o.request_end_ns = now_ns();
    o.cpu_end_100ns = process_cpu_100ns();
    o.request_allocation_counter_end =
        g_allocation_count.load(std::memory_order_relaxed);
    g_count_allocations.store(false, std::memory_order_release);

    if (o.trace.count > 0u) {
        o.decode_allocation_counter_start = o.trace.allocation_count_at_ready[0];
        o.decode_allocation_counter_end =
            o.trace.allocation_count_at_ready[o.trace.count - 1u];
    }
    return o;
}

Observation run_ring(const RunRequest& request) {
    using Ring = arcllm::research::lmax_p1::SequencedRing<RuntimeEvent, 64>;
    Observation o;
    o.ring_applicable = true;

    Ring ring;
    std::atomic<bool> consumer_ready{false};
    std::atomic<bool> done{false};
    WaitCountersAtomic ready_counters;
    WaitCountersAtomic completion_counters;
    char error_buffer[1024]{};

    std::thread consumer([&] {
        publish_true_and_wake(consumer_ready, ready_counters);
        const RuntimeEvent event = ring.consume();
        if (event.sequence != 0u) ring.record_sequence_gap();
        try {
            *event.result = invoke_canonical_semantics(*event.request, *event.trace);
        } catch (const std::exception& e) {
            if (event.error && event.error_capacity) {
#ifdef _WIN32
                strncpy_s(event.error, event.error_capacity, e.what(), _TRUNCATE);
#else
                std::snprintf(event.error, event.error_capacity, "%s", e.what());
#endif
            }
        } catch (...) {
            if (event.error && event.error_capacity) {
#ifdef _WIN32
                strncpy_s(event.error, event.error_capacity, "unknown exception", _TRUNCATE);
#else
                std::snprintf(event.error, event.error_capacity, "%s", "unknown exception");
#endif
            }
        }
        publish_true_and_wake(*event.done, completion_counters);
    });

    // Frozen v0.3 rule: consumer-ready handshake completes before measured
    // CPU/request timing and uses the same low-duty wait policy.
    wait_until_true(consumer_ready, ready_counters);

    g_allocation_count.store(0u, std::memory_order_relaxed);
    o.request_allocation_counter_start =
        g_allocation_count.load(std::memory_order_relaxed);
    g_count_allocations.store(true, std::memory_order_release);
    o.cpu_start_100ns = process_cpu_100ns();
    o.request_start_ns = now_ns();

    RuntimeEvent event;
    event.sequence = 0u;
    event.request = &request;
    event.result = &o.result;
    event.trace = &o.trace;
    event.done = &done;
    event.error = error_buffer;
    event.error_capacity = sizeof(error_buffer);
    ring.publish(event);

    // Frozen v0.3 completion policy: 16 YieldProcessor spins, one
    // SwitchToThread, then WaitOnAddress until the consumer publishes done.
    wait_until_true(done, completion_counters);

    o.request_end_ns = now_ns();
    o.cpu_end_100ns = process_cpu_100ns();
    o.request_allocation_counter_end =
        g_allocation_count.load(std::memory_order_relaxed);
    g_count_allocations.store(false, std::memory_order_release);

    o.ring = ring.counters();
    consumer.join();
    o.completion_wait = completion_counters.snapshot();
    o.consumer_ready_wait = ready_counters.snapshot();

    if (o.trace.count > 0u) {
        o.decode_allocation_counter_start = o.trace.allocation_count_at_ready[0];
        o.decode_allocation_counter_end =
            o.trace.allocation_count_at_ready[o.trace.count - 1u];
    }
    if (error_buffer[0] != '\0') {
        o.error = error_buffer;
        o.success = false;
    } else {
        o.success = true;
    }
    return o;
}

bool semantic_trace_consistent(const Observation& o, const Workload& w) {
    if (!o.success || o.trace.overflow) return false;
    if (o.result.generated_token_ids.size() != w.max_new_tokens) return false;
    if (o.trace.count != w.max_new_tokens) return false;
    for (std::size_t i = 0; i < o.result.generated_token_ids.size(); ++i) {
        if (o.trace.token_ids[i] != o.result.generated_token_ids[i]) return false;
        if (i && o.trace.ready_ns[i] < o.trace.ready_ns[i - 1u]) return false;
    }
    return o.result.stats.finite;
}

void write_result(
    const std::string& path,
    const std::string& arm,
    const std::string& phase,
    const std::int64_t pair_index,
    const std::int64_t pair_position,
    const Workload& w,
    const Observation& o,
    const IdentityEvidence& identity) {

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot open --out path");

    const bool trace_ok = semantic_trace_consistent(o, w);
    const std::uint64_t wall_ns =
        o.request_end_ns >= o.request_start_ns ? o.request_end_ns - o.request_start_ns : 0u;
    const std::uint64_t ttft_ns =
        o.trace.count ? o.trace.ready_ns[0] - o.request_start_ns : 0u;
    const std::uint64_t cpu_100ns =
        o.cpu_end_100ns >= o.cpu_start_100ns ? o.cpu_end_100ns - o.cpu_start_100ns : 0u;
    const double cpu_ns = static_cast<double>(cpu_100ns) * 100.0;
    const double cpu_util =
        wall_ns ? (cpu_ns / (static_cast<double>(wall_ns) * logical_processor_count())) * 100.0 : 0.0;
    double decode_tps = 0.0;
    if (o.trace.count > 1u && o.trace.ready_ns[o.trace.count - 1u] > o.trace.ready_ns[0]) {
        const double decode_seconds =
            static_cast<double>(o.trace.ready_ns[o.trace.count - 1u] - o.trace.ready_ns[0]) / 1.0e9;
        decode_tps = static_cast<double>(o.trace.count - 1u) / decode_seconds;
    }

    const auto& s = o.result.stats;
    out << std::setprecision(15);
    out << "{\n";
    out << "  \"schema\":\"arcllm.lmax_arch_p1.inference_observation.v0.4\",\n";
    out << "  \"arm\":\"" << arm << "\",\"phase\":\"" << phase
        << "\",\"cell\":\"" << w.id << "\","
        << "\"pair_index\":" << pair_index
        << ",\"pair_position\":" << pair_position << ",";
    out << "\"prefill_tokens\":" << w.prefill_tokens
        << ",\"max_new_tokens\":" << w.max_new_tokens << ",\n";
    out << "  \"success\":" << (o.success ? "true" : "false")
        << ",\"semantic_trace_consistent\":" << (trace_ok ? "true" : "false")
        << ",\"error\":\"" << escape_json(o.error) << "\",\n";
    const std::uint64_t allocation_count_request =
        o.request_allocation_counter_end >= o.request_allocation_counter_start
            ? o.request_allocation_counter_end - o.request_allocation_counter_start : 0u;
    const std::uint64_t allocation_count_decode =
        o.decode_allocation_counter_end >= o.decode_allocation_counter_start
            ? o.decode_allocation_counter_end - o.decode_allocation_counter_start : 0u;

    out << "  \"identity\":{"
        << "\"runner_sha256\":\"" << escape_json(identity.runner_sha256) << "\","
        << "\"model_sha256\":\"" << escape_json(identity.model_sha256) << "\","
        << "\"shader_manifest_sha256\":\"" << escape_json(identity.shader_manifest_sha256) << "\","
        << "\"authorization_sha256\":\"" << escape_json(identity.authorization_sha256) << "\","
        << "\"evidence_contract_sha256\":\"" << escape_json(identity.evidence_contract_sha256) << "\"},\n";
    out << "  \"raw_primitives\":{"
        << "\"request_start_ns\":" << o.request_start_ns
        << ",\"request_end_ns\":" << o.request_end_ns
        << ",\"first_token_ready_ns\":" << (o.trace.count ? o.trace.ready_ns[0] : 0u)
        << ",\"cpu_start_100ns\":" << o.cpu_start_100ns
        << ",\"cpu_end_100ns\":" << o.cpu_end_100ns
        << ",\"logical_processor_count\":" << logical_processor_count()
        << ",\"request_allocation_counter_start\":" << o.request_allocation_counter_start
        << ",\"request_allocation_counter_end\":" << o.request_allocation_counter_end
        << ",\"decode_allocation_counter_start\":" << o.decode_allocation_counter_start
        << ",\"decode_allocation_counter_end\":" << o.decode_allocation_counter_end
        << ",\"monotonic_clock\":\"std::chrono::steady_clock\"},\n";
    out << "  \"metrics\":{\"ttft_ns\":" << ttft_ns
        << ",\"e2e_ns\":" << wall_ns
        << ",\"decode_tokens_per_second\":" << decode_tps
        << ",\"cpu_utilization_percent\":" << cpu_util
        << ",\"allocation_count_request\":" << allocation_count_request
        << ",\"allocation_count_decode_window\":" << allocation_count_decode
        << "},\n";
    out << "  \"token_ready_ns\":[";
    for (std::uint32_t i = 0; i < o.trace.count; ++i) {
        if (i) out << ",";
        out << o.trace.ready_ns[i];
    }
    out << "],\n  \"itl_ns\":[";
    for (std::uint32_t i = 1; i < o.trace.count; ++i) {
        if (i > 1u) out << ",";
        out << (o.trace.ready_ns[i] - o.trace.ready_ns[i - 1u]);
    }
    out << "],\n  \"generated_token_ids\":[";
    for (std::size_t i = 0; i < o.result.generated_token_ids.size(); ++i) {
        if (i) out << ",";
        out << o.result.generated_token_ids[i];
    }
    out << "],\n";
    out << "  \"generated_token_count\":" << o.result.generated_token_ids.size() << ",\n";
    out << "  \"runtime_stats\":{"
        << "\"prefill_dispatches\":" << s.prefill_dispatches
        << ",\"prefill_submits\":" << s.prefill_submits
        << ",\"decode_dispatches_per_step\":" << s.decode_dispatches_per_step
        << ",\"decode_submits_per_step\":" << s.decode_submits_per_step
        << ",\"decode_steps\":" << s.decode_steps
        << ",\"route_a_steps\":" << s.route_a_steps
        << ",\"route_b_steps\":" << s.route_b_steps
        << ",\"acquire_events\":" << s.acquire_events
        << ",\"evict_events\":" << s.evict_events
        << ",\"b_allocations\":" << s.b_allocations
        << ",\"b_materializations\":" << s.b_materializations
        << ",\"b_validations\":" << s.b_validations
        << ",\"b_releases\":" << s.b_releases
        << ",\"p1_calls\":" << s.p1_calls
        << ",\"p3_calls\":" << s.p3_calls
        << ",\"p0_calls\":" << s.p0_calls
        << ",\"finite\":" << (s.finite ? "true" : "false") << "},\n";
    if (!o.ring_applicable) {
        out << "  \"ring\":null,\n";
        out << "  \"completion_wait\":null,\n";
        out << "  \"consumer_ready_wait\":null,\n";
    } else {
        out << "  \"ring\":{"
            << "\"capacity\":64"
            << ",\"publish_count\":" << o.ring.publish_count
            << ",\"consume_count\":" << o.ring.consume_count
            << ",\"max_occupancy\":" << o.ring.max_occupancy
            << ",\"full_backpressure_observations\":" << o.ring.full_backpressure_observations
            << ",\"slot_spin_count\":" << o.ring.slot_spin_count
            << ",\"slot_switch_to_thread_count\":" << o.ring.slot_switch_to_thread_count
            << ",\"slot_wait_on_address_count\":" << o.ring.slot_wait_on_address_count
            << ",\"slot_wake_count\":" << o.ring.slot_wake_count
            << ",\"sequence_gap_count\":" << o.ring.sequence_gap_count << "},\n";
        out << "  \"completion_wait\":{"
            << "\"spin_count\":" << o.completion_wait.spin_count
            << ",\"switch_to_thread_count\":" << o.completion_wait.switch_to_thread_count
            << ",\"wait_on_address_count\":" << o.completion_wait.wait_on_address_count
            << ",\"wake_count\":" << o.completion_wait.wake_count << "},\n";
        out << "  \"consumer_ready_wait\":{"
            << "\"spin_count\":" << o.consumer_ready_wait.spin_count
            << ",\"switch_to_thread_count\":" << o.consumer_ready_wait.switch_to_thread_count
            << ",\"wait_on_address_count\":" << o.consumer_ready_wait.wait_on_address_count
            << ",\"wake_count\":" << o.consumer_ready_wait.wake_count << "},\n";
    }
    out << "  \"governance\":{"
        << "\"evidence_profile\":0,"
        << "\"request_within_validated_domain\":false,"
        << "\"greedy_generation\":true,"
        << "\"execution_authorization_verified\":true"
        << "}\n}\n";
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
        std::string arm, phase, cell, model, shader_dir, sidecar, out_path, authorization_path;
        std::int64_t pair_index = -1;
        std::int64_t pair_position = -1;
        IdentityEvidence identity;
        bool describe = false;
        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto need = [&](const char* flag) {
                if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + flag);
                return std::string(argv[++i]);
            };
            if (a == "--describe") describe = true;
            else if (a == "--arm") arm = need("--arm");
            else if (a == "--phase") phase = need("--phase");
            else if (a == "--pair-index") pair_index = std::stoll(need("--pair-index"));
            else if (a == "--pair-position") pair_position = std::stoll(need("--pair-position"));
            else if (a == "--cell") cell = need("--cell");
            else if (a == "--model") model = need("--model");
            else if (a == "--shader-dir") shader_dir = need("--shader-dir");
            else if (a == "--sidecar") sidecar = need("--sidecar");
            else if (a == "--out") out_path = need("--out");
            else if (a == "--authorization") authorization_path = need("--authorization");
            else if (a == "--runner-sha256") identity.runner_sha256 = need("--runner-sha256");
            else if (a == "--model-sha256") identity.model_sha256 = need("--model-sha256");
            else if (a == "--shader-manifest-sha256") identity.shader_manifest_sha256 = need("--shader-manifest-sha256");
            else if (a == "--authorization-sha256") identity.authorization_sha256 = need("--authorization-sha256");
            else if (a == "--evidence-contract-sha256") identity.evidence_contract_sha256 = need("--evidence-contract-sha256");
            else throw std::runtime_error("unknown argument: " + a);
        }

        if (describe) {
            std::cout
                << "ARCLLM_LMAX_ARCH_P1_RUNNER=BUILDONLY_DESCRIBE\n"
                << "ARMS=CURRENT_DIRECT,LMAX_RING_P1\n"
                << "WAIT_POLICY=16_YIELDPROCESSOR_1_SWITCHTOTHREAD_WAITONADDRESS\n"
                << "RAW_PRIMITIVES_SERIALIZED=true\n"
                << "CELLS=W1,W2,W3,W4,W5,W6\n"
                << "PROFILE=0\n"
                << "WITHIN_VALIDATED_DOMAIN=false\n"
                << "MODEL_EXECUTED=false\n"
                << "VULKAN_INITIALIZED=false\n";
            return 0;
        }

        if (!execution_authorized(authorization_path)) {
            throw std::runtime_error(
                "outcome execution blocked: missing or invalid P1 E execution authorization");
        }
        const bool measured_identity_ok =
            phase == "measured" &&
            pair_index >= 0 && pair_index <= 3 &&
            pair_position >= 0 && pair_position <= 1;
        const bool warmup_identity_ok =
            phase == "warmup" && pair_index == -1 && pair_position == -1;
        if ((arm != "direct" && arm != "ring") ||
            (!measured_identity_ok && !warmup_identity_ok) ||
            cell.empty() || model.empty() || shader_dir.empty() || out_path.empty() ||
            identity.runner_sha256.empty() || identity.model_sha256.empty() ||
            identity.shader_manifest_sha256.empty() ||
            identity.authorization_sha256.empty() ||
            identity.evidence_contract_sha256.empty()) {
            throw std::runtime_error(
                "required: --authorization FILE --arm direct|ring --phase warmup|measured --pair-index --pair-position --cell W1..W6 --model --shader-dir --out plus five frozen identity hashes");
        }

        const Workload w = workload_for(cell);
        RunRequest request;
        request.model_path = model;
        request.shader_dir = shader_dir;
        request.sidecar_path = sidecar;
        request.input_token_ids = make_tokens(w);
        request.max_new_tokens = w.max_new_tokens;
        request.evidence_profile = arcllm::v1::runtime::EvidenceProfile::PROFILE_0;
        request.request_within_validated_domain = false;

        Observation observation =
            arm == "direct" ? run_direct(request) : run_ring(request);
        write_result(
            out_path,
            arm == "direct" ? "CURRENT_DIRECT" : "LMAX_RING_P1",
            phase,
            pair_index,
            pair_position,
            w,
            observation,
            identity);
        return observation.success && semantic_trace_consistent(observation, w) ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "ARCLLM_LMAX_ARCH_P1_RUNNER_ERROR=" << e.what() << "\n";
        return 2;
    }
}
