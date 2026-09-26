#pragma once
#include "arcllm_v1_b1_policy.h"

namespace arcllm::v1::b1 {

struct RuntimeSignals {
    Workload workload = Workload::WS;
    bool current_request_in_validated_domain = true;
    bool model_loaded = true;

    bool b_resident = false;
    bool b_identity_valid = false;
    bool b_execution_available = true;
    bool residency_lease_granted = false;

    bool future_reuse_known = false;
    std::uint64_t future_reuse_tokens = 0;

    bool acquisition_allowed = true;
    bool p1_available = true;
    bool p1_vetoed = false;
    bool p3_cold_available = false;
    bool p0_cpu_available = true;
};

struct RuntimePlan {
    Decision decision{};
    bool route_a = true;
    bool route_b_after_success = false;
    bool acquire_p1 = false;
    bool acquire_p3_cold = false;
    bool acquire_p0_cpu = false;
    bool evict_b = false;
    bool require_post_acquisition_identity_validation = false;
    std::uint64_t requested_residency_bytes = kExec148ResidentBytes;
};

constexpr Snapshot to_snapshot(const RuntimeSignals& x) {
    Snapshot s{};
    s.workload = x.workload;
    s.current_request_in_validated_domain = x.current_request_in_validated_domain;
    s.model_loaded = x.model_loaded;
    s.b_resident = x.b_resident;
    s.b_identity_valid = x.b_identity_valid;
    s.b_execution_available = x.b_execution_available;
    s.residency_lease_granted = x.residency_lease_granted;
    s.future_reuse_known = x.future_reuse_known;
    s.future_reuse_tokens = x.future_reuse_tokens;
    s.acquisition_allowed = x.acquisition_allowed;
    s.p1_available = x.p1_available;
    s.p1_vetoed = x.p1_vetoed;
    s.p3_cold_available = x.p3_cold_available;
    s.p0_cpu_available = x.p0_cpu_available;
    return s;
}

constexpr RuntimePlan make_runtime_plan(const RuntimeSignals& x) {
    const Decision d = evaluate(to_snapshot(x));
    RuntimePlan p{};
    p.decision = d;
    p.route_a = d.route == Route::A_SPLIT_K32;
    p.route_b_after_success = d.route == Route::B_EXEC148;
    p.acquire_p1 = d.lifecycle == Lifecycle::ACQUIRE_B_P1_GPU;
    p.acquire_p3_cold = d.lifecycle == Lifecycle::ACQUIRE_B_P3_COLD;
    p.acquire_p0_cpu = d.lifecycle == Lifecycle::ACQUIRE_B_P0_CPU;
    p.evict_b = d.lifecycle == Lifecycle::EVICT_B;
    p.require_post_acquisition_identity_validation =
        p.acquire_p1 || p.acquire_p3_cold || p.acquire_p0_cpu;
    p.requested_residency_bytes = kExec148ResidentBytes;
    return p;
}

// Runtime integration contract:
// 1. Resource manager decides residency_lease_granted; this hook invents no RAM threshold.
// 2. Acquisition actions are single plans, not automatic retry chains.
// 3. Runtime may route B only after the requested acquisition completes and exact identity
//    validation succeeds; otherwise it routes A and reevaluates later with updated availability.
// 4. EVICT_B releases exactly kExec148ResidentBytes of B residency before continuing on A.
// 5. OUTSIDE_VALIDATED_DOMAIN may route A while preserve_existing_b=true.
class SelectionLifetimeHook {
public:
    constexpr RuntimePlan plan(const RuntimeSignals& signals) const {
        return make_runtime_plan(signals);
    }
};

} // namespace arcllm::v1::b1
