#include "../include/arcllm/v1/package_api.h"
#include "../include/arcllm/v1/backend_api.h"
#include "arcllm_v1_b1_policy_runtime_hook.h"

namespace arcllm::v1::package {
namespace {

using b1::Workload;

constexpr CapabilityDescriptor kDescriptor{
    CapabilityId::Q4K_DECODE_FFN_DOWN,
    kApiVersion,
    PrimitiveId::A_SPLIT_K32,
    PrimitiveId::B_EXEC148,
    b1::kExec148ResidentBytes,
    {
        {EvidenceProfileId::PROFILE_0, 2ull, 15ull, 17ull},
        {EvidenceProfileId::PROFILE_1, 4ull, 33ull, 37ull}
    },
    2u
};

constexpr bool supported_capability(CapabilityId id) noexcept {
    return id == CapabilityId::Q4K_DECODE_FFN_DOWN;
}

constexpr bool to_workload(EvidenceProfileId id, Workload& out) noexcept {
    if (id == EvidenceProfileId::PROFILE_0) {
        out = Workload::WS;
        return true;
    }
    if (id == EvidenceProfileId::PROFILE_1) {
        out = Workload::WC;
        return true;
    }
    return false;
}

constexpr PlanReason map_reason(b1::Reason r) noexcept {
    switch (r) {
        case b1::Reason::OUTSIDE_VALIDATED_DOMAIN: return PlanReason::OUTSIDE_VALIDATED_DOMAIN;
        case b1::Reason::RESIDENT_B_INVALID: return PlanReason::RESIDENT_B_INVALID;
        case b1::Reason::RESIDENCY_LEASE_REVOKED: return PlanReason::RESIDENCY_LEASE_REVOKED;
        case b1::Reason::NO_FUTURE_REUSE: return PlanReason::NO_FUTURE_REUSE;
        case b1::Reason::RESIDENT_B_REUSED: return PlanReason::RESIDENT_B_REUSED;
        case b1::Reason::FUTURE_REUSE_UNKNOWN: return PlanReason::FUTURE_REUSE_UNKNOWN;
        case b1::Reason::RESIDENCY_LEASE_DENIED: return PlanReason::RESIDENCY_LEASE_DENIED;
        case b1::Reason::ACQUISITION_VETOED: return PlanReason::ACQUISITION_VETOED;
        case b1::Reason::P1_THRESHOLD_MET: return PlanReason::PRIMARY_THRESHOLD_MET;
        case b1::Reason::P1_BELOW_THRESHOLD: return PlanReason::PRIMARY_BELOW_THRESHOLD;
        case b1::Reason::P3_COLD_THRESHOLD_MET: return PlanReason::SECONDARY_THRESHOLD_MET;
        case b1::Reason::P3_COLD_BELOW_THRESHOLD: return PlanReason::SECONDARY_BELOW_THRESHOLD;
        case b1::Reason::P0_THRESHOLD_MET: return PlanReason::TERTIARY_THRESHOLD_MET;
        case b1::Reason::MODEL_UNAVAILABLE: return PlanReason::MODEL_UNAVAILABLE;
        case b1::Reason::NO_JUSTIFIED_B_ACQUISITION_PATH:
        default: return PlanReason::NO_JUSTIFIED_B_ACQUISITION_PATH;
    }
}

constexpr PrimitiveId map_route(b1::Route r) noexcept {
    return r == b1::Route::B_EXEC148 ? PrimitiveId::B_EXEC148 : PrimitiveId::A_SPLIT_K32;
}

constexpr AcquisitionId map_acquisition(b1::Lifecycle x) noexcept {
    switch (x) {
        case b1::Lifecycle::ACQUIRE_B_P1_GPU: return AcquisitionId::GPU_IN_PLACE;
        case b1::Lifecycle::ACQUIRE_B_P3_COLD: return AcquisitionId::COLD_UNBUFFERED_SIDECAR;
        case b1::Lifecycle::ACQUIRE_B_P0_CPU: return AcquisitionId::CPU_DIRECT;
        default: return AcquisitionId::NONE;
    }
}

constexpr LifecycleAction map_lifecycle(b1::Lifecycle x) noexcept {
    if (x == b1::Lifecycle::EVICT_B) return LifecycleAction::EVICT;
    if (x == b1::Lifecycle::ACQUIRE_B_P1_GPU ||
        x == b1::Lifecycle::ACQUIRE_B_P3_COLD ||
        x == b1::Lifecycle::ACQUIRE_B_P0_CPU) {
        return LifecycleAction::ACQUIRE;
    }
    return LifecycleAction::NONE;
}

Plan fallback_unsupported(PlanStatus status) noexcept {
    Plan p{};
    p.status = status;
    p.route = PrimitiveId::A_SPLIT_K32;
    p.lifecycle = LifecycleAction::NONE;
    p.acquisition = AcquisitionId::NONE;
    p.reason = PlanReason::UNSUPPORTED_REQUEST;
    p.requested_residency_bytes = b1::kExec148ResidentBytes;
    return p;
}

BackendStatus resolve_route(
    PrimitiveId route,
    BackendAdapter& backend,
    ApplyResult& result,
    bool preserve_existing_failure = false) noexcept {

    const BackendStatus prior = result.backend_status;
    PrimitiveHandle h{};
    const BackendStatus s = backend.resolve_primitive(route, h);
    if (!(preserve_existing_failure && prior != BackendStatus::OK)) {
        result.backend_status = s;
    }
    if (s == BackendStatus::OK && static_cast<bool>(h)) {
        result.primitive = h;
        result.ready = true;
    }
    return s;
}

BackendStatus fallback_a(
    BackendAdapter& backend,
    ApplyResult& result,
    bool preserve_existing_failure = true) noexcept {

    result.fell_back_to_a = true;
    return resolve_route(
        PrimitiveId::A_SPLIT_K32,
        backend,
        result,
        preserve_existing_failure);
}

} // namespace

const CapabilityDescriptor& capability_descriptor() noexcept {
    return kDescriptor;
}

Plan plan(const PlanRequest& request) noexcept {
    if (!supported_capability(request.capability)) {
        return fallback_unsupported(PlanStatus::UNSUPPORTED_CAPABILITY);
    }

    Workload workload{};
    if (!to_workload(request.profile, workload)) {
        return fallback_unsupported(PlanStatus::UNSUPPORTED_PROFILE);
    }

    b1::RuntimeSignals x{};
    x.workload = workload;
    x.current_request_in_validated_domain = request.runtime.current_request_in_validated_domain;
    x.model_loaded = request.runtime.model_loaded;
    x.b_resident = request.runtime.b_resident;
    x.b_identity_valid = request.runtime.b_identity_valid;
    x.b_execution_available = request.runtime.b_execution_available;
    x.residency_lease_granted = request.runtime.residency_lease_granted;
    x.future_reuse_known = request.runtime.future_reuse.known;
    x.future_reuse_tokens = request.runtime.future_reuse.tokens;
    x.acquisition_allowed = request.runtime.acquisition_allowed;
    x.p1_available = request.runtime.primary_acquisition_available;
    x.p1_vetoed = request.runtime.primary_acquisition_vetoed;
    x.p3_cold_available = request.runtime.secondary_acquisition_available;
    x.p0_cpu_available = request.runtime.tertiary_acquisition_available;

    const b1::RuntimePlan internal = b1::make_runtime_plan(x);

    Plan p{};
    p.status = PlanStatus::OK;
    p.route = map_route(internal.decision.route);
    p.lifecycle = map_lifecycle(internal.decision.lifecycle);
    p.acquisition = map_acquisition(internal.decision.lifecycle);
    p.reason = map_reason(internal.decision.reason);
    p.threshold_tokens = internal.decision.threshold_tokens;
    p.requested_residency_bytes = internal.requested_residency_bytes;
    p.preserve_existing_b = internal.decision.preserve_existing_b;
    p.require_post_acquisition_identity_validation =
        internal.require_post_acquisition_identity_validation;
    return p;
}

ApplyResult apply_plan(
    const Plan& requested,
    BackendAdapter& backend,
    PackageState& state) noexcept {

    ApplyResult result{};
    result.plan = requested;

    if (requested.status != PlanStatus::OK) {
        fallback_a(backend, result, false);
        return result;
    }

    if (requested.lifecycle == LifecycleAction::EVICT) {
        if (state.b_representation) {
            const BackendStatus release_status = backend.release(state.b_representation);
            state.b_representation = {};
            if (release_status != BackendStatus::OK) {
                result.backend_status = release_status;
                fallback_a(backend, result, true);
                return result;
            }
        }
        fallback_a(backend, result, false);
        return result;
    }

    if (requested.lifecycle == LifecycleAction::ACQUIRE) {
        RepresentationHandle candidate{};
        BackendStatus s = backend.acquire(
            requested.acquisition,
            requested.requested_residency_bytes,
            candidate);
        result.backend_status = s;
        if (s != BackendStatus::OK || !candidate) {
            fallback_a(backend, result, true);
            return result;
        }

        if (!requested.require_post_acquisition_identity_validation) {
            backend.release(candidate);
            result.backend_status = BackendStatus::VALIDATION_FAILED;
            fallback_a(backend, result, true);
            return result;
        }

        s = backend.validate(candidate, CapabilityId::Q4K_DECODE_FFN_DOWN);
        result.backend_status = s;
        if (s != BackendStatus::OK) {
            backend.release(candidate);
            fallback_a(backend, result, true);
            return result;
        }

        state.b_representation = candidate;
        s = resolve_route(PrimitiveId::B_EXEC148, backend, result, false);
        if (s != BackendStatus::OK) {
            backend.release(state.b_representation);
            state.b_representation = {};
            fallback_a(backend, result, true);
        }
        return result;
    }

    if (requested.route == PrimitiveId::B_EXEC148 && !state.b_representation) {
        result.backend_status = BackendStatus::UNAVAILABLE;
        fallback_a(backend, result, true);
        return result;
    }

    resolve_route(requested.route, backend, result, false);
    return result;
}

} // namespace arcllm::v1::package
