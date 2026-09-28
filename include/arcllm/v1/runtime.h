#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace arcllm::v1::runtime {

enum class EvidenceProfile : std::uint8_t {
    PROFILE_0 = 0,
    PROFILE_1 = 1
};

struct RunRequest {
    std::string model_path;
    std::string shader_dir;
    std::string sidecar_path;
    std::vector<std::uint32_t> input_token_ids;
    std::uint32_t max_new_tokens = 1;
    EvidenceProfile evidence_profile = EvidenceProfile::PROFILE_0;
    // This is an evidence boundary, not a prompt classifier. Callers must only
    // set true for requests covered by the currently validated runtime domain.
    bool request_within_validated_domain = false;
};

struct RuntimeStats {
    std::uint32_t prefill_dispatches = 0;
    std::uint32_t prefill_submits = 0;
    std::uint32_t decode_dispatches_per_step = 0;
    std::uint32_t decode_submits_per_step = 0;
    std::uint32_t decode_steps = 0;

    std::uint32_t route_a_steps = 0;
    std::uint32_t route_b_steps = 0;
    std::uint32_t acquire_events = 0;
    std::uint32_t evict_events = 0;

    std::uint64_t b_allocations = 0;
    std::uint64_t b_materializations = 0;
    std::uint64_t b_validations = 0;
    std::uint64_t b_releases = 0;
    std::uint64_t p1_calls = 0;
    std::uint64_t p3_calls = 0;
    std::uint64_t p0_calls = 0;

    bool finite = false;
};

struct RunResult {
    std::vector<std::uint32_t> generated_token_ids;
    RuntimeStats stats{};
};

// Reusable canonical inference entry point for the currently supported exact
// ArcLLM-v1 model/runtime domain. It performs greedy generation and contains no
// experiment fixture, expected token hash, performance adjudication, or PASS/FAIL
// decision logic.
RunResult generate(const RunRequest& request);

} // namespace arcllm::v1::runtime
