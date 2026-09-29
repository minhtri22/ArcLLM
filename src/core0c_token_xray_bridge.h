#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "token_xray_arcllm_v1_adapter.h"
#include "token_xray_runtime_lifecycle_trace.h"

namespace arcllm_core0c {

static constexpr const char* kModelSha =
    "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463";

inline std::string env_or_empty(const char* key) {
    const char* v = std::getenv(key);
    return v ? std::string(v) : std::string();
}

inline bool enabled() {
    return !env_or_empty("ARCLLM_CORE0C_TRACE_DIR").empty();
}

inline std::string run_id() {
    auto v = env_or_empty("ARCLLM_CORE0C_RUN_ID");
    if (v.empty()) throw std::runtime_error("CORE0C trace enabled without ARCLLM_CORE0C_RUN_ID");
    return v;
}

inline std::string request_id() {
    auto v = env_or_empty("ARCLLM_CORE0C_REQUEST_ID");
    return v.empty() ? run_id() : v;
}

inline std::string trace_dir() {
    auto v = env_or_empty("ARCLLM_CORE0C_TRACE_DIR");
    if (v.empty()) throw std::runtime_error("CORE0C trace directory is empty");
    std::filesystem::create_directories(v);
    return v;
}

inline double timestamp_period_ns() {
    auto v = env_or_empty("ARCLLM_CORE0C_TIMESTAMP_PERIOD_NS");
    if (v.empty()) throw std::runtime_error("CORE0C timestamp period is not frozen");
    const double x = std::stod(v);
    if (!(x > 0.0)) throw std::runtime_error("CORE0C timestamp period must be >0");
    return x;
}

inline uint32_t expected_timestamp_valid_bits() {
    auto v = env_or_empty("ARCLLM_CORE0C_TIMESTAMP_VALID_BITS");
    if (v.empty()) throw std::runtime_error("CORE0C timestamp valid bits are not frozen");
    const auto x = static_cast<uint32_t>(std::stoul(v));
    if (x == 0 || x > 64) throw std::runtime_error("CORE0C timestamp valid bits invalid");
    return x;
}

inline std::string esc(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '\\') o << "\\\\";
        else if (c == '"') o << "\\\"";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

struct DispatchObservation {
    uint32_t dispatch_id = 0;
    std::string runtime_name;
    std::string shader_path;
    uint32_t group_x = 1;
    uint32_t group_y = 1;
    uint32_t group_z = 1;
    uint64_t start_tick = 0;
    uint64_t end_tick = 0;
    uint64_t duration_ns = 0;
};

inline std::string token_trace_filename(const std::string& mode, uint32_t index) {
    std::ostringstream s;
    s << trace_dir() << "\\" << run_id() << "_" << mode << "_"
      << std::setw(3) << std::setfill('0') << index << ".json";
    return s.str();
}

inline void write_token_trace(
    const std::string& mode,
    uint32_t token_index,
    uint32_t context_length,
    uint32_t timestamp_valid_bits,
    double timestamp_period,
    uint64_t device_span_ns,
    const std::vector<DispatchObservation>& dispatches) {

    if (!enabled()) return;
    if (mode != "prefill" && mode != "decode")
        throw std::runtime_error("CORE0C invalid token trace mode");
    if (timestamp_valid_bits != expected_timestamp_valid_bits())
        throw std::runtime_error("CORE0C timestampValidBits drift");
    if (std::abs(timestamp_period - timestamp_period_ns()) > 1e-12)
        throw std::runtime_error("CORE0C timestampPeriod drift");

    uint64_t sum = 0;
    for (const auto& d : dispatches) sum += d.duration_ns;
    const uint64_t unattributed = device_span_ns > sum ? device_span_ns - sum : 0;

    const auto path = token_trace_filename(mode, token_index);
    std::ofstream o(path, std::ios::binary | std::ios::trunc);
    if (!o) throw std::runtime_error("CORE0C cannot write TOKEN_TRACE");

    o << "{\n";
    o << "  \"schema_version\": \"0.2\",\n";
    o << "  \"artifact_type\": \"TOKEN_TRACE\",\n";
    o << "  \"run_id\": \"" << esc(run_id()) << "\",\n";
    o << "  \"source_model\": {\"sha256\": \"" << kModelSha << "\"},\n";
    o << "  \"token\": {\"mode\": \"" << mode << "\", \"token_index\": "
      << token_index << ", \"context_length\": " << context_length
      << ", \"input_token_id\": null, \"output_token_id\": null},\n";
    o << "  \"runtime\": {\"name\": \"ArcLLM-v1\", \"version\": \"CORE0C_DIAGNOSTIC\", \"backend\": \"vulkan\"},\n";
    o << "  \"measurement_context\": {\"measurement_mode\": \"TOKEN_XRAY_TRACE\", "
      << "\"timing_authority\": \"DIAGNOSTIC_ONLY\", \"instrumentation_overhead\": null},\n";
    o << "  \"device\": {\"profile_id\": \"arc140v-dev-host\", "
      << "\"execution_domain_id\": \"gpu.arc_140v\", "
      << "\"live_identity\": {\"timestamp_period_ns\": " << timestamp_period
      << ", \"timestamp_valid_bits\": " << timestamp_valid_bits << "}},\n";
    o << "  \"timing\": {\"source\": \"VULKAN_TIMESTAMP_QUERY\", "
      << "\"timestamp_period_ns\": " << timestamp_period
      << ", \"timestamp_valid_bits\": " << timestamp_valid_bits
      << ", \"device_span_ns\": " << device_span_ns
      << ", \"sum_dispatch_duration_ns\": " << sum
      << ", \"unattributed_device_time_ns\": " << unattributed << "},\n";
    o << "  \"submissions\": [{\"submit_id\": 0, \"queue\": 0, "
      << "\"command_buffer\": \"primary\", \"dispatch_ids\": [";
    for (size_t i = 0; i < dispatches.size(); ++i) {
        if (i) o << ", ";
        o << dispatches[i].dispatch_id;
    }
    o << "]}],\n";
    o << "  \"barriers\": [],\n";
    o << "  \"dispatches\": [\n";

    for (size_t i = 0; i < dispatches.size(); ++i) {
        const auto& d = dispatches[i];
        const auto ids = token_xray::arcllm_v1_semantic_node_ids(d.runtime_name, mode);
        const auto local = token_xray::arcllm_v1_local_size(d.shader_path);
        const auto subgroup = token_xray::arcllm_v1_subgroup_size(d.shader_path);
        o << "    {\n";
        o << "      \"dispatch_id\": " << d.dispatch_id << ",\n";
        o << "      \"semantic_node_ids\": [";
        for (size_t j = 0; j < ids.size(); ++j) {
            if (j) o << ", ";
            o << "\"" << esc(ids[j]) << "\"";
        }
        o << "],\n";
        o << "      \"runtime_name\": \"" << esc(d.runtime_name) << "\",\n";
        o << "      \"kernel\": \"" << esc(token_xray::arcllm_v1_basename(d.shader_path)) << "\",\n";
        o << "      \"shader_path\": \"" << esc(d.shader_path) << "\",\n";
        o << "      \"backend\": \"vulkan\", \"queue\": 0, \"command_buffer\": \"primary\",\n";
        o << "      \"workgroups\": [" << d.group_x << ", " << d.group_y << ", " << d.group_z << "],\n";
        o << "      \"local_size\": [" << local[0] << ", " << local[1] << ", " << local[2] << "],\n";
        o << "      \"subgroup_size\": " << (subgroup ? std::to_string(subgroup) : "null") << ",\n";
        o << "      \"query_indices\": {\"start\": " << (2 + 2*d.dispatch_id)
          << ", \"end\": " << (3 + 2*d.dispatch_id) << "},\n";
        o << "      \"timestamp\": {\"source\": \"VULKAN_TIMESTAMP_QUERY\", "
          << "\"start_ns\": null, \"end_ns\": null, \"duration_ns\": " << d.duration_ns
          << ", \"start_tick\": " << d.start_tick << ", \"end_tick\": " << d.end_tick
          << ", \"timestamp_period_ns\": " << timestamp_period
          << ", \"timestamp_valid_bits\": " << timestamp_valid_bits << "},\n";
        o << "      \"barrier_before\": " << (i ? "true" : "false")
          << ", \"barrier_after\": " << (i + 1 < dispatches.size() ? "true" : "true") << ",\n";
        o << "      \"barrier_before_kind\": " << (i ? "\"compute_to_compute\"" : "null")
          << ", \"barrier_after_kind\": "
          << (i + 1 < dispatches.size() ? "\"compute_to_compute\"" : "\"compute_to_host\"") << ",\n";
        o << "      \"counters\": {}\n";
        o << "    }" << (i + 1 == dispatches.size() ? "\n" : ",\n");
    }
    o << "  ]\n}\n";
}

class LifecycleCollector {
public:
    void add(
        std::string event_type,
        std::string runtime_name,
        std::string representation_id,
        std::string resource_id,
        std::string execution_domain_id = {},
        std::vector<uint32_t> related_dispatch_ids = {},
        int64_t duration_ns = -1,
        std::string provenance_class = "SYSTEM_OBSERVED",
        std::string provenance_note = {}) {
        if (!enabled()) return;
        token_xray::RuntimeLifecycleEventSpec e;
        e.event_type = std::move(event_type);
        e.runtime_name = std::move(runtime_name);
        e.representation_id = std::move(representation_id);
        e.resource_id = std::move(resource_id);
        e.execution_domain_id = std::move(execution_domain_id);
        e.related_dispatch_ids = std::move(related_dispatch_ids);
        e.duration_ns = duration_ns;
        e.timestamp_source = duration_ns >= 0 ? "VULKAN_TIMESTAMP_QUERY" : "";
        e.provenance_class = std::move(provenance_class);
        e.provenance_source_id = "ArcLLM-CORE0C";
        e.provenance_note = std::move(provenance_note);
        events_.push_back(std::move(e));
    }

    void flush() {
        if (!enabled()) return;
        token_xray::RuntimeLifecycleMeta meta;
        meta.run_id = run_id();
        meta.request_id = request_id();
        meta.model_sha256 = kModelSha;
        meta.hardware_profile_id = "arc140v-dev-host";
        meta.runtime_name = "ArcLLM-v1";
        meta.runtime_version = "CORE0C_DIAGNOSTIC";
        meta.backend = "vulkan";
        token_xray::RuntimeLifecycleTrace trace(std::move(meta));
        for (auto& e : events_) trace.add_event(std::move(e));
        const auto path = trace_dir() + "\\" + run_id() + "_runtime_lifecycle.json";
        trace.write_json(path);
        events_.clear();
    }

private:
    std::vector<token_xray::RuntimeLifecycleEventSpec> events_;
};

inline LifecycleCollector& lifecycle() {
    static LifecycleCollector c;
    return c;
}

inline void record_p1_materialization(
    const std::vector<std::string>& runtime_names,
    const std::vector<uint64_t>& durations_ns) {
    if (!enabled()) return;
    if (runtime_names.size() != durations_ns.size())
        throw std::runtime_error("CORE0C P1 materialization trace shape mismatch");
    for (size_t i = 0; i < runtime_names.size(); ++i) {
        const auto a = token_xray::arcllm_v1_q4v4_lifecycle_event(runtime_names[i]);
        lifecycle().add(
            a.event_type, a.runtime_name, a.representation_id, a.resource_id,
            a.execution_domain_id, {static_cast<uint32_t>(i)},
            static_cast<int64_t>(durations_ns[i]), "MEASURED",
            "P1 GPU materialization dispatch");
    }
}

inline void record_acquire() {
    lifecycle().add("representation_acquire", "Q4V4.P1", "q4v4.exec148",
                    "q4v4.exec148.family", {}, {}, -1, "SYSTEM_OBSERVED",
                    "generic policy selected ACQUIRE");
}
inline void record_validate() {
    lifecycle().add("representation_validate", "Q4V4", "q4v4.exec148",
                    "q4v4.exec148.family", {}, {}, -1, "SYSTEM_OBSERVED",
                    "exact image validation completed");
}
inline void record_resident() {
    lifecycle().add("representation_resident", "Q4V4", "q4v4.exec148",
                    "q4v4.exec148.family", {}, {}, -1, "SYSTEM_OBSERVED",
                    "validated representation became resident");
}
inline void record_reuse() {
    lifecycle().add("representation_reuse", "Q4V4", "q4v4.exec148",
                    "q4v4.exec148.family", {}, {}, -1, "SYSTEM_OBSERVED",
                    "resident validated representation reused");
}
inline void record_evict() {
    lifecycle().add("representation_evict", "Q4V4", "q4v4.exec148",
                    "q4v4.exec148.family", {}, {}, -1, "SYSTEM_OBSERVED",
                    "generic policy selected EVICT");
}
inline void record_release() {
    lifecycle().add("representation_release", "Q4V4", "q4v4.exec148",
                    "q4v4.exec148.family", {}, {}, -1, "SYSTEM_OBSERVED",
                    "representation buffer released");
}

} // namespace arcllm_core0c
