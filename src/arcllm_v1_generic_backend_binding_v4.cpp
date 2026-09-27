#include "../include/arcllm/v1/generic_backend_binding_v4.h"

namespace arcllm::v1::binding_v4 {

RepresentationHandle BindingState::find(PrimitiveId primitive) const noexcept {
    for (const auto& slot : slots)
        if (slot.primitive == primitive) return slot.representation;
    return {};
}

bool BindingState::set(PrimitiveId primitive, RepresentationHandle representation) noexcept {
    for (auto& slot : slots) {
        if (slot.primitive == primitive || !slot.primitive) {
            slot.primitive = primitive;
            slot.representation = representation;
            return true;
        }
    }
    return false;
}

void BindingState::clear(PrimitiveId primitive) noexcept {
    for (auto& slot : slots) {
        if (slot.primitive == primitive) {
            slot = {};
            return;
        }
    }
}

namespace {

BackendStatus resolve_route(
    PrimitiveId route,
    BackendAdapter& backend,
    ApplyResult& result) noexcept {
    if (!route) return BackendStatus::OK;
    PrimitiveHandle handle{};
    const BackendStatus s = backend.resolve_primitive(route, handle);
    result.backend_status = s;
    if (s == BackendStatus::OK && static_cast<bool>(handle)) {
        result.primitive = handle;
        result.ready = true;
    } else if (s == BackendStatus::OK) {
        result.backend_status = BackendStatus::PRIMITIVE_UNAVAILABLE;
    }
    return result.backend_status;
}

} // namespace

ApplyResult apply_decision(
    const PrimitiveRegistry& registry,
    CapabilityId capability,
    const PolicyDecision& decision,
    BackendAdapter& backend,
    BindingState& state) noexcept {

    ApplyResult result{};
    result.decision = decision;

    const auto* cap = registry.find_capability(capability);
    if (!cap) {
        result.backend_status = BackendStatus::INVALID_BINDING_STATE;
        return result;
    }
    const auto* preferred = registry.find_primitive(cap->preferred_primitive);
    if (!preferred) {
        result.backend_status = BackendStatus::INVALID_BINDING_STATE;
        return result;
    }

    if (decision.lifecycle == policy_v4::LifecycleAction::EVICT) {
        const RepresentationHandle existing = state.find(preferred->id);
        if (existing) {
            const BackendStatus release_status = backend.release(preferred->id, existing);
            result.cleanup_status = release_status;
            if (release_status != BackendStatus::OK) {
                result.backend_status = release_status;
                result.unreleased_representation = existing;
                return result;
            }
            state.clear(preferred->id);
        }
        return resolve_route(decision.route, backend, result), result;
    }

    if (decision.lifecycle == policy_v4::LifecycleAction::ACQUIRE) {
        if (!decision.acquisition || decision.route != preferred->id) {
            result.backend_status = BackendStatus::INVALID_BINDING_STATE;
            return result;
        }

        RepresentationHandle candidate{};
        BackendStatus s = backend.acquire(
            decision.acquisition,
            preferred->id,
            decision.requested_residency_bytes,
            candidate);
        result.backend_status = s;
        if (s != BackendStatus::OK || !candidate) {
            if (s == BackendStatus::OK) result.backend_status = BackendStatus::ACQUISITION_FAILED;
            return result;
        }

        if (decision.require_post_acquisition_identity_validation) {
            s = backend.validate(candidate, capability, preferred->id);
            result.backend_status = s;
            if (s != BackendStatus::OK) {
                const BackendStatus cleanup = backend.release(preferred->id, candidate);
                result.cleanup_status = cleanup;
                if (cleanup != BackendStatus::OK) result.unreleased_representation = candidate;
                return result;
            }
        }

        if (!state.set(preferred->id, candidate)) {
            result.backend_status = BackendStatus::INVALID_BINDING_STATE;
            const BackendStatus cleanup = backend.release(preferred->id, candidate);
            result.cleanup_status = cleanup;
            if (cleanup != BackendStatus::OK) result.unreleased_representation = candidate;
            return result;
        }

        s = resolve_route(decision.route, backend, result);
        if (s != BackendStatus::OK) {
            const RepresentationHandle stored = state.find(preferred->id);
            if (stored) {
                const BackendStatus cleanup = backend.release(preferred->id, stored);
                result.cleanup_status = cleanup;
                if (cleanup == BackendStatus::OK) state.clear(preferred->id);
                else result.unreleased_representation = stored;
            }
        }
        return result;
    }

    return resolve_route(decision.route, backend, result), result;
}

} // namespace arcllm::v1::binding_v4
