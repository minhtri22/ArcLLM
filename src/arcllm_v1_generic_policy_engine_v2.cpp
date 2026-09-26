#include "../include/arcllm/v1/generic_policy_engine_v2.h"

namespace arcllm::v1::policy_v2 {
namespace {

using registry_v2::AcquisitionPathDescriptor;
using registry_v2::AcquisitionThresholdDescriptor;
using registry_v2::AcquisitionTrigger;
using registry_v2::LifecycleDescriptor;
using registry_v2::PrimitiveDescriptor;
using registry_v2::ValidationState;

bool active(ValidationState s) noexcept { return s==ValidationState::VALIDATED; }

bool valid_runtime_arrays(const PolicyRequest&r) noexcept {
    if(r.primitive_state_count&&!r.primitive_states) return false;
    if(r.acquisition_state_count&&!r.acquisition_states) return false;
    for(std::size_t i=0;i<r.primitive_state_count;++i){
        if(!r.primitive_states[i].primitive) return false;
        for(std::size_t j=i+1;j<r.primitive_state_count;++j)
            if(r.primitive_states[i].primitive==r.primitive_states[j].primitive) return false;
    }
    for(std::size_t i=0;i<r.acquisition_state_count;++i){
        if(!r.acquisition_states[i].acquisition) return false;
        for(std::size_t j=i+1;j<r.acquisition_state_count;++j)
            if(r.acquisition_states[i].acquisition==r.acquisition_states[j].acquisition) return false;
    }
    return true;
}

const PrimitiveRuntimeState* runtime_primitive(const PolicyRequest&r,PrimitiveId id) noexcept {
    for(std::size_t i=0;i<r.primitive_state_count;++i)
        if(r.primitive_states[i].primitive==id) return &r.primitive_states[i];
    return nullptr;
}
const AcquisitionRuntimeState* runtime_acquisition(const PolicyRequest&r,AcquisitionPathId id) noexcept {
    for(std::size_t i=0;i<r.acquisition_state_count;++i)
        if(r.acquisition_states[i].acquisition==id) return &r.acquisition_states[i];
    return nullptr;
}

PolicyDecision routed(
    PrimitiveId route,PolicyReason reason,PolicyStatus status=PolicyStatus::OK,bool preserve=false) noexcept {
    PolicyDecision d{};d.status=status;d.route=route;d.reason=reason;d.preserve_existing_representation=preserve;return d;
}
PolicyDecision no_route(PolicyStatus status,PolicyReason reason,bool preserve=false) noexcept {
    return routed({},reason,status,preserve);
}

struct Candidate {
    const AcquisitionPathDescriptor* path=nullptr;
    const AcquisitionThresholdDescriptor* threshold=nullptr;
    const PrimitiveDescriptor* target=nullptr;
};
bool better(const Candidate&a,const Candidate&b) noexcept {
    if(!b.path) return true;
    if(a.path->priority!=b.path->priority) return a.path->priority<b.path->priority;
    return a.path->id.value<b.path->id.value;
}

PolicyDecision fallback_or_unavailable(
    const PrimitiveDescriptor* fallback,
    PolicyReason fallback_reason,
    PolicyStatus no_fallback_status,
    PolicyReason no_fallback_reason,
    bool preserve=false) noexcept {
    if(fallback) return routed(fallback->id,fallback_reason,PolicyStatus::OK,preserve);
    return no_route(no_fallback_status,no_fallback_reason,preserve);
}

PolicyDecision evict_decision(
    const PrimitiveDescriptor* fallback,
    PolicyReason reason) noexcept {
    PolicyDecision d=fallback?
        routed(fallback->id,reason):
        no_route(PolicyStatus::NOT_READY,reason);
    d.lifecycle=LifecycleAction::EVICT;
    return d;
}

} // namespace

PolicyDecision evaluate(const PrimitiveRegistry& registry,const PolicyRequest&r) noexcept {
    if(!valid_runtime_arrays(r))
        return no_route(PolicyStatus::INVALID_RUNTIME_STATE,PolicyReason::UNSUPPORTED_REQUEST);

    const auto*cap=registry.find_capability(r.capability);
    if(!cap||!active(cap->state))
        return no_route(PolicyStatus::UNSUPPORTED_CAPABILITY,PolicyReason::UNSUPPORTED_REQUEST);

    const auto*profile=registry.find_profile(r.profile);
    if(!profile||!active(profile->state))
        return no_route(PolicyStatus::UNSUPPORTED_PROFILE,PolicyReason::UNSUPPORTED_REQUEST);
    if(profile->domain!=cap->domain)
        return no_route(PolicyStatus::PROFILE_DOMAIN_MISMATCH,PolicyReason::UNSUPPORTED_REQUEST);

    const auto*domain=registry.find_domain(cap->domain);
    const auto*evidence=registry.find_evidence_set(profile->evidence_set);
    if(!domain||!evidence||!active(domain->state)||!active(evidence->state))
        return no_route(PolicyStatus::REGISTRY_POLICY_INCOMPLETE,PolicyReason::UNSUPPORTED_REQUEST);
    if(profile->reuse_metric){
        const auto*metric=registry.find_reuse_metric(profile->reuse_metric);
        if(!metric||!active(metric->state))
            return no_route(PolicyStatus::REGISTRY_POLICY_INCOMPLETE,PolicyReason::UNSUPPORTED_REQUEST);
    }

    const auto*preferred=registry.find_primitive(cap->preferred_primitive);
    if(!preferred||!active(preferred->state)||preferred->capability!=cap->id)
        return no_route(PolicyStatus::REGISTRY_POLICY_INCOMPLETE,PolicyReason::UNSUPPORTED_REQUEST);

    const PrimitiveDescriptor*fallback=nullptr;
    if(cap->fallback_primitive){
        fallback=registry.find_primitive(cap->fallback_primitive);
        if(!fallback||!active(fallback->state)||fallback->capability!=cap->id)
            return no_route(PolicyStatus::REGISTRY_POLICY_INCOMPLETE,PolicyReason::UNSUPPORTED_REQUEST);
    }

    const LifecycleDescriptor*lifecycle=registry.find_lifecycle(preferred->id);
    if((preferred->flags&registry_v2::PRIMITIVE_REQUIRES_RESIDENCY)&&!lifecycle)
        return no_route(PolicyStatus::REGISTRY_POLICY_INCOMPLETE,PolicyReason::UNSUPPORTED_REQUEST);

    const auto*pr=runtime_primitive(r,preferred->id);
    if(pr&&pr->resident){
        if(!r.model_loaded&&lifecycle&&lifecycle->evict_on_model_unload)
            return evict_decision(fallback,PolicyReason::MODEL_UNAVAILABLE);
        if(!pr->identity_valid&&lifecycle&&lifecycle->evict_on_identity_invalid)
            return evict_decision(fallback,PolicyReason::RESIDENT_PRIMITIVE_INVALID);
        if(!pr->execution_available&&lifecycle&&lifecycle->evict_on_execution_unavailable)
            return evict_decision(fallback,PolicyReason::RESIDENT_EXECUTION_UNAVAILABLE);
        if(!pr->residency_lease_granted&&lifecycle&&lifecycle->evict_on_lease_revoke)
            return evict_decision(fallback,PolicyReason::RESIDENCY_LEASE_REVOKED);
        if(r.future_reuse_known&&r.future_reuse_units==0&&lifecycle&&lifecycle->evict_on_zero_future_reuse)
            return evict_decision(fallback,PolicyReason::NO_FUTURE_REUSE);

        if(!r.request_within_capability_evidence_scope){
            const bool preserve=lifecycle&&lifecycle->preserve_residency_outside_domain;
            return fallback_or_unavailable(
                fallback,
                PolicyReason::OUTSIDE_VALIDATED_DOMAIN,
                PolicyStatus::OUTSIDE_VALIDATED_CAPABILITY,
                PolicyReason::OUTSIDE_EVIDENCE_SCOPE_NO_FALLBACK,
                preserve);
        }

        PolicyDecision d{};
        d.route=preferred->id;
        d.reason=PolicyReason::RESIDENT_PRIMITIVE_REUSED;
        d.preserve_existing_representation=true;
        return d;
    }

    if(!r.model_loaded)
        return fallback_or_unavailable(
            fallback,PolicyReason::MODEL_UNAVAILABLE,
            PolicyStatus::NOT_READY,PolicyReason::MODEL_UNAVAILABLE);

    if(!r.request_within_capability_evidence_scope)
        return fallback_or_unavailable(
            fallback,PolicyReason::OUTSIDE_VALIDATED_DOMAIN,
            PolicyStatus::OUTSIDE_VALIDATED_CAPABILITY,
            PolicyReason::OUTSIDE_EVIDENCE_SCOPE_NO_FALLBACK);

    if(!r.acquisition_allowed)
        return fallback_or_unavailable(
            fallback,PolicyReason::ACQUISITION_GLOBALLY_VETOED,
            PolicyStatus::NOT_READY,PolicyReason::MANDATORY_ACQUISITION_UNAVAILABLE);

    Candidate best{};
    Candidate best_below{};
    bool saw_active=false;
    bool saw_runtime_available=false;
    bool saw_mandatory=false;
    bool saw_mandatory_blocked=false;
    bool saw_amortized=false;
    bool saw_amortized_semantic_block=false;
    bool registry_incomplete=false;

    for(std::size_t b=0;b<registry.bundle_count();++b){
        const auto*bundle=registry.bundle_at(b);
        if(!bundle) continue;
        for(std::size_t i=0;i<bundle->acquisition_count;++i){
            const auto&path=bundle->acquisitions[i];
            if(!active(path.state)||path.target_primitive!=preferred->id) continue;
            saw_active=true;
            if(path.trigger==AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY) saw_mandatory=true;
            else saw_amortized=true;

            const auto*ar=runtime_acquisition(r,path.id);
            if(!ar||!ar->available||ar->vetoed){
                if(path.trigger==AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY) saw_mandatory_blocked=true;
                continue;
            }
            saw_runtime_available=true;

            const auto*target_runtime=runtime_primitive(r,path.target_primitive);
            if(!target_runtime||!target_runtime->residency_lease_granted){
                if(path.trigger==AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY) saw_mandatory_blocked=true;
                continue;
            }

            const auto*target=registry.find_primitive(path.target_primitive);
            if(!target||!active(target->state)||target->capability!=cap->id){registry_incomplete=true;continue;}

            if(path.trigger==AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY){
                Candidate c{&path,nullptr,target};
                if(better(c,best)) best=c;
                continue;
            }

            if(!profile->reuse_metric){registry_incomplete=true;continue;}
            if(!r.future_reuse_known||r.future_reuse_units==0){
                saw_amortized_semantic_block=true;
                continue;
            }

            const auto*threshold=registry.find_threshold(cap->id,profile->id,path.id);
            if(!threshold||!active(threshold->state)||threshold->reuse_metric!=profile->reuse_metric){
                registry_incomplete=true;continue;
            }

            Candidate c{&path,threshold,target};
            if(r.future_reuse_units<threshold->minimum_reuse_units){
                if(better(c,best_below)) best_below=c;
                continue;
            }
            if(better(c,best)) best=c;
        }
    }

    if(best.path){
        PolicyDecision d{};
        d.route=best.target->id;
        d.lifecycle=LifecycleAction::ACQUIRE;
        d.acquisition=best.path->id;
        d.reason=best.path->trigger==AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY?
            PolicyReason::MANDATORY_ACQUISITION_REQUIRED:
            PolicyReason::ACQUISITION_THRESHOLD_MET;
        d.threshold_units=best.threshold?best.threshold->minimum_reuse_units:0;
        d.requested_residency_bytes=best.target->residency_bytes;
        d.require_post_acquisition_identity_validation=
            (best.target->flags&registry_v2::PRIMITIVE_REQUIRES_IDENTITY_VALIDATION)!=0;
        return d;
    }

    if(registry_incomplete)
        return no_route(PolicyStatus::REGISTRY_POLICY_INCOMPLETE,PolicyReason::UNSUPPORTED_REQUEST);

    if(fallback){
        if(best_below.path){
            PolicyDecision d=routed(fallback->id,PolicyReason::BELOW_ALL_ACTIVE_THRESHOLDS);
            d.threshold_units=best_below.threshold->minimum_reuse_units;
            return d;
        }
        if(saw_amortized_semantic_block){
            if(!r.future_reuse_known) return routed(fallback->id,PolicyReason::FUTURE_REUSE_UNKNOWN);
            if(r.future_reuse_units==0) return routed(fallback->id,PolicyReason::NO_FUTURE_REUSE);
        }
        if(saw_active&&!saw_runtime_available)
            return routed(fallback->id,PolicyReason::NO_AVAILABLE_ACQUISITION_PATH);
        return routed(fallback->id,PolicyReason::NO_AVAILABLE_ACQUISITION_PATH);
    }

    if(saw_mandatory||saw_mandatory_blocked)
        return no_route(PolicyStatus::NOT_READY,PolicyReason::MANDATORY_ACQUISITION_UNAVAILABLE);
    if(saw_amortized||saw_amortized_semantic_block)
        return no_route(PolicyStatus::NOT_READY,PolicyReason::NO_AVAILABLE_ACQUISITION_PATH);
    return no_route(PolicyStatus::NOT_READY,PolicyReason::NO_AVAILABLE_ACQUISITION_PATH);
}

} // namespace arcllm::v1::policy_v2
