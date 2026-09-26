#include "../include/arcllm/v1/generic_policy_engine.h"

namespace arcllm::v1::policy {
namespace {

using registry::AcquisitionPathDescriptor;
using registry::AcquisitionThresholdDescriptor;
using registry::LifecycleDescriptor;
using registry::PrimitiveDescriptor;
using registry::ResidentPreferenceDescriptor;
using registry::ValidationState;

bool active(ValidationState s) noexcept {
    return s == ValidationState::VALIDATED;
}

bool valid_runtime_arrays(const PolicyRequest& r) noexcept {
    if (r.primitive_state_count && !r.primitive_states) return false;
    if (r.acquisition_state_count && !r.acquisition_states) return false;

    for (std::size_t i=0;i<r.primitive_state_count;++i) {
        if (!r.primitive_states[i].primitive) return false;
        for (std::size_t j=i+1;j<r.primitive_state_count;++j)
            if (r.primitive_states[i].primitive == r.primitive_states[j].primitive) return false;
    }
    for (std::size_t i=0;i<r.acquisition_state_count;++i) {
        if (!r.acquisition_states[i].acquisition) return false;
        for (std::size_t j=i+1;j<r.acquisition_state_count;++j)
            if (r.acquisition_states[i].acquisition == r.acquisition_states[j].acquisition) return false;
    }
    return true;
}

const PrimitiveRuntimeState* runtime_primitive(
    const PolicyRequest& r, PrimitiveId id) noexcept {

    for (std::size_t i=0;i<r.primitive_state_count;++i)
        if (r.primitive_states[i].primitive == id) return &r.primitive_states[i];
    return nullptr;
}

const AcquisitionRuntimeState* runtime_acquisition(
    const PolicyRequest& r, AcquisitionPathId id) noexcept {

    for (std::size_t i=0;i<r.acquisition_state_count;++i)
        if (r.acquisition_states[i].acquisition == id) return &r.acquisition_states[i];
    return nullptr;
}

PolicyDecision fallback(
    PrimitiveId primitive,
    PolicyReason reason,
    PolicyStatus status = PolicyStatus::OK,
    bool preserve = false) noexcept {

    PolicyDecision d{};
    d.status = status;
    d.route = primitive;
    d.reason = reason;
    d.preserve_existing_representation = preserve;
    return d;
}

PolicyDecision evict(
    PrimitiveId fallback_primitive,
    PrimitiveId represented,
    PolicyReason reason) noexcept {

    PolicyDecision d = fallback(fallback_primitive, reason);
    d.lifecycle = LifecycleAction::EVICT;
    d.requested_residency_bytes = 0;
    d.preserve_existing_representation = false;
    (void)represented;
    return d;
}

struct Candidate {
    const AcquisitionPathDescriptor* path = nullptr;
    const AcquisitionThresholdDescriptor* threshold = nullptr;
    const PrimitiveDescriptor* target = nullptr;
};

bool better_candidate(const Candidate& a, const Candidate& b) noexcept {
    if (!b.path) return true;
    if (a.path->priority != b.path->priority) return a.path->priority < b.path->priority;
    return a.path->id.value < b.path->id.value;
}

} // namespace

PolicyDecision evaluate(
    const PrimitiveRegistry& registry,
    const PolicyRequest& r) noexcept {

    if (!valid_runtime_arrays(r)) {
        return fallback({}, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::INVALID_RUNTIME_STATE);
    }

    const auto* capability = registry.find_capability(r.capability);
    if (!capability || !active(capability->state)) {
        return fallback({}, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::UNSUPPORTED_CAPABILITY);
    }

    const auto* fallback_primitive = registry.find_primitive(capability->fallback_primitive);
    if (!fallback_primitive || !active(fallback_primitive->state)) {
        return fallback({}, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::REGISTRY_POLICY_INCOMPLETE);
    }

    const auto* profile = registry.find_profile(r.profile);
    if (!profile || !active(profile->state)) {
        return fallback(capability->fallback_primitive, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::UNSUPPORTED_PROFILE);
    }
    if (profile->domain != capability->domain) {
        return fallback(capability->fallback_primitive, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::PROFILE_DOMAIN_MISMATCH);
    }

    const auto* domain = registry.find_domain(capability->domain);
    const auto* metric = registry.find_reuse_metric(profile->reuse_metric);
    const auto* evidence = registry.find_evidence_set(profile->evidence_set);
    if (!domain || !metric || !evidence ||
        !active(domain->state) || !active(metric->state) || !active(evidence->state)) {
        return fallback(capability->fallback_primitive, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::REGISTRY_POLICY_INCOMPLETE);
    }

    const ResidentPreferenceDescriptor* pref =
        registry.find_resident_preference(capability->id, profile->id);

    if (!pref || !active(pref->state)) {
        return fallback(capability->fallback_primitive, PolicyReason::FALLBACK_ONLY);
    }
    if (pref->fallback_primitive != capability->fallback_primitive) {
        return fallback(capability->fallback_primitive, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::REGISTRY_POLICY_INCOMPLETE);
    }

    const auto* represented = registry.find_primitive(pref->preferred_when_resident);
    if (!represented || !active(represented->state) || represented->capability != capability->id) {
        return fallback(capability->fallback_primitive, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::REGISTRY_POLICY_INCOMPLETE);
    }

    const LifecycleDescriptor* lifecycle = registry.find_lifecycle(represented->id);
    if (!lifecycle) {
        return fallback(capability->fallback_primitive, PolicyReason::UNSUPPORTED_REQUEST, PolicyStatus::REGISTRY_POLICY_INCOMPLETE);
    }

    const PrimitiveRuntimeState* represented_runtime = runtime_primitive(r, represented->id);

    if (represented_runtime && represented_runtime->resident) {
        if (!r.model_loaded && lifecycle->evict_on_model_unload)
            return evict(capability->fallback_primitive, represented->id, PolicyReason::MODEL_UNAVAILABLE);

        if (!represented_runtime->identity_valid && lifecycle->evict_on_identity_invalid)
            return evict(capability->fallback_primitive, represented->id, PolicyReason::RESIDENT_PRIMITIVE_INVALID);

        if (!represented_runtime->execution_available && lifecycle->evict_on_execution_unavailable)
            return evict(capability->fallback_primitive, represented->id, PolicyReason::RESIDENT_EXECUTION_UNAVAILABLE);

        if (!represented_runtime->residency_lease_granted && lifecycle->evict_on_lease_revoke)
            return evict(capability->fallback_primitive, represented->id, PolicyReason::RESIDENCY_LEASE_REVOKED);

        if (r.future_reuse_known && r.future_reuse_units == 0 && lifecycle->evict_on_zero_future_reuse)
            return evict(capability->fallback_primitive, represented->id, PolicyReason::NO_FUTURE_REUSE);

        if (!r.current_request_in_validated_domain) {
            return fallback(
                capability->fallback_primitive,
                PolicyReason::OUTSIDE_VALIDATED_DOMAIN,
                PolicyStatus::OK,
                lifecycle->preserve_residency_outside_domain);
        }

        PolicyDecision d{};
        d.route = represented->id;
        d.reason = PolicyReason::RESIDENT_PRIMITIVE_REUSED;
        d.preserve_existing_representation = true;
        return d;
    }

    if (!r.model_loaded)
        return fallback(capability->fallback_primitive, PolicyReason::MODEL_UNAVAILABLE);
    if (!r.current_request_in_validated_domain)
        return fallback(capability->fallback_primitive, PolicyReason::OUTSIDE_VALIDATED_DOMAIN);
    if (!r.future_reuse_known)
        return fallback(capability->fallback_primitive, PolicyReason::FUTURE_REUSE_UNKNOWN);
    if (r.future_reuse_units == 0)
        return fallback(capability->fallback_primitive, PolicyReason::NO_FUTURE_REUSE);
    if (!r.acquisition_allowed)
        return fallback(capability->fallback_primitive, PolicyReason::ACQUISITION_GLOBALLY_VETOED);

    Candidate best{};
    Candidate best_below{};
    bool saw_active_path = false;
    bool saw_runtime_available_path = false;

    for (std::size_t b=0;b<registry.bundle_count();++b) {
        const auto* bundle = registry.bundle_at(b);
        if (!bundle) continue;

        for (std::size_t i=0;i<bundle->acquisition_count;++i) {
            const auto& path = bundle->acquisitions[i];
            if (!active(path.state)) continue;
            if (path.target_primitive != represented->id) continue;
            saw_active_path = true;

            const auto* ar = runtime_acquisition(r, path.id);
            if (!ar || !ar->available || ar->vetoed) continue;
            saw_runtime_available_path = true;

            const auto* target_runtime = runtime_primitive(r, path.target_primitive);
            if (!target_runtime || !target_runtime->residency_lease_granted) continue;

            const auto* threshold = registry.find_threshold(capability->id, profile->id, path.id);
            if (!threshold || !active(threshold->state)) continue;
            if (threshold->reuse_metric != profile->reuse_metric) continue;

            const auto* target = registry.find_primitive(path.target_primitive);
            if (!target || !active(target->state) || target->capability != capability->id) continue;

            Candidate c{&path, threshold, target};
            if (r.future_reuse_units < threshold->minimum_reuse_units) {
                if (better_candidate(c, best_below)) best_below = c;
                continue;
            }

            if (better_candidate(c, best)) best = c;
        }
    }

    if (!best.path) {
        if (best_below.path) {
            PolicyDecision d = fallback(
                capability->fallback_primitive,
                PolicyReason::BELOW_ALL_ACTIVE_THRESHOLDS);
            d.threshold_units = best_below.threshold->minimum_reuse_units;
            return d;
        }
        if (saw_active_path && !saw_runtime_available_path)
            return fallback(capability->fallback_primitive, PolicyReason::NO_AVAILABLE_ACQUISITION_PATH);
        return fallback(capability->fallback_primitive, PolicyReason::NO_AVAILABLE_ACQUISITION_PATH);
    }

    PolicyDecision d{};
    d.route = best.target->id;
    d.lifecycle = LifecycleAction::ACQUIRE;
    d.acquisition = best.path->id;
    d.reason = PolicyReason::ACQUISITION_THRESHOLD_MET;
    d.threshold_units = best.threshold->minimum_reuse_units;
    d.requested_residency_bytes = best.target->residency_bytes;
    d.require_post_acquisition_identity_validation =
        (best.target->flags & registry::PRIMITIVE_REQUIRES_IDENTITY_VALIDATION) != 0;
    return d;
}

} // namespace arcllm::v1::policy
