#include "../include/arcllm/v1/generic_policy_engine.h"
#include "../src/registrations/arcllm_v1_q4k_down_reference_registration.h"
#include "../src/arcllm_v1_b1_policy_runtime_hook.h"
#include <array>
#include <cstdlib>
#include <iostream>

using namespace arcllm::v1;
namespace reg = arcllm::v1::registry;
namespace gp = arcllm::v1::policy;
namespace ref = arcllm::v1::reference_registration;

static void req(bool c,const char*m){if(!c){std::cerr<<"GENERIC_POLICY_FAIL: "<<m<<"\n";std::exit(2);}}

static gp::LifecycleAction map_lifecycle(b1::Lifecycle x){
    if(x==b1::Lifecycle::EVICT_B) return gp::LifecycleAction::EVICT;
    if(x==b1::Lifecycle::ACQUIRE_B_P1_GPU||x==b1::Lifecycle::ACQUIRE_B_P3_COLD||x==b1::Lifecycle::ACQUIRE_B_P0_CPU)
        return gp::LifecycleAction::ACQUIRE;
    return gp::LifecycleAction::NONE;
}
static reg::AcquisitionPathId map_acquisition(b1::Lifecycle x){
    if(x==b1::Lifecycle::ACQUIRE_B_P1_GPU) return ref::kAcquirePrimary;
    if(x==b1::Lifecycle::ACQUIRE_B_P3_COLD) return ref::kAcquireSecondary;
    if(x==b1::Lifecycle::ACQUIRE_B_P0_CPU) return ref::kAcquireTertiary;
    return {};
}
static reg::PrimitiveId map_route(b1::Route x){
    return x==b1::Route::B_EXEC148?ref::kPrimitiveB:ref::kPrimitiveA;
}

static void reference_equivalence(){
    reg::PrimitiveRegistry registry;
    reg::RegistryError err{};
    req(registry.add_bundle(ref::q4k_down_reference_bundle(),&err)==reg::RegistryStatus::OK,"reference registration");

    constexpr std::array<std::uint64_t,14> H{0,1,2,3,4,14,15,16,17,32,33,36,37,64};
    std::uint64_t cases=0;

    for(int wi=0;wi<2;++wi){
        for(auto h:H){
            for(std::uint32_t mask=0;mask<(1u<<12);++mask){
                b1::RuntimeSignals old{};
                old.workload=wi==0?b1::Workload::WS:b1::Workload::WC;
                old.current_request_in_validated_domain=(mask&(1u<<0))!=0;
                old.model_loaded=(mask&(1u<<1))!=0;
                old.b_resident=(mask&(1u<<2))!=0;
                old.b_identity_valid=(mask&(1u<<3))!=0;
                old.b_execution_available=(mask&(1u<<4))!=0;
                old.residency_lease_granted=(mask&(1u<<5))!=0;
                old.future_reuse_known=(mask&(1u<<6))!=0;
                old.future_reuse_tokens=h;
                old.acquisition_allowed=(mask&(1u<<7))!=0;
                old.p1_available=(mask&(1u<<8))!=0;
                old.p1_vetoed=(mask&(1u<<9))!=0;
                old.p3_cold_available=(mask&(1u<<10))!=0;
                old.p0_cpu_available=(mask&(1u<<11))!=0;

                const auto od=b1::make_runtime_plan(old);

                gp::PrimitiveRuntimeState ps[]{
                    {ref::kPrimitiveB,old.b_resident,old.b_identity_valid,old.b_execution_available,old.residency_lease_granted}
                };
                gp::AcquisitionRuntimeState as[]{
                    {ref::kAcquirePrimary,old.p1_available,old.p1_vetoed},
                    {ref::kAcquireSecondary,old.p3_cold_available,false},
                    {ref::kAcquireTertiary,old.p0_cpu_available,false},
                    {ref::kAcquireDisabledWarm,true,false}
                };
                gp::PolicyRequest r{};
                r.capability=ref::kCapability;
                r.profile=wi==0?ref::kProfile0:ref::kProfile1;
                r.current_request_in_validated_domain=old.current_request_in_validated_domain;
                r.model_loaded=old.model_loaded;
                r.future_reuse_known=old.future_reuse_known;
                r.future_reuse_units=h;
                r.acquisition_allowed=old.acquisition_allowed;
                r.primitive_states=ps;r.primitive_state_count=1;
                r.acquisition_states=as;r.acquisition_state_count=4;

                const auto nd=gp::evaluate(registry,r);
                req(nd.status==gp::PolicyStatus::OK,"reference generic status");
                req(nd.route==map_route(od.decision.route),"reference route equivalence");
                req(nd.lifecycle==map_lifecycle(od.decision.lifecycle),"reference lifecycle equivalence");
                req(nd.acquisition==map_acquisition(od.decision.lifecycle),"reference acquisition equivalence");
                if (od.decision.threshold_tokens != 0) {
                    req(nd.threshold_units==od.decision.threshold_tokens,"reference emitted-threshold equivalence");
                }
                req(nd.preserve_existing_representation==od.decision.preserve_existing_b,"reference preserve equivalence");
                if(nd.lifecycle==gp::LifecycleAction::ACQUIRE){
                    req(nd.requested_residency_bytes==549527552ull,"reference acquisition residency");
                    req(nd.require_post_acquisition_identity_validation,"reference acquisition validation");
                }
                ++cases;
            }
        }
    }
    req(cases==114688,"reference equivalence case count");

    // Legacy B1 leaves tertiary below-threshold diagnostic metadata at zero.
    // Generic policy normalizes that metadata from the registry without changing
    // route/lifecycle/acquisition.
    {
        b1::RuntimeSignals old{};
        old.workload=b1::Workload::WS;
        old.current_request_in_validated_domain=true;
        old.model_loaded=true;
        old.b_resident=false;
        old.residency_lease_granted=true;
        old.future_reuse_known=true;
        old.future_reuse_tokens=16;
        old.acquisition_allowed=true;
        old.p1_available=false;
        old.p3_cold_available=false;
        old.p0_cpu_available=true;
        const auto legacy=b1::make_runtime_plan(old);
        req(legacy.decision.route==b1::Route::A_SPLIT_K32 &&
            legacy.decision.lifecycle==b1::Lifecycle::NONE &&
            legacy.decision.threshold_tokens==0,
            "legacy tertiary below-threshold diagnostic baseline");

        gp::PrimitiveRuntimeState ps[]={{ref::kPrimitiveB,false,false,true,true}};
        gp::AcquisitionRuntimeState as[]={
            {ref::kAcquirePrimary,false,false},
            {ref::kAcquireSecondary,false,false},
            {ref::kAcquireTertiary,true,false}
        };
        gp::PolicyRequest r{};
        r.capability=ref::kCapability;r.profile=ref::kProfile0;
        r.current_request_in_validated_domain=true;r.model_loaded=true;
        r.future_reuse_known=true;r.future_reuse_units=16;r.acquisition_allowed=true;
        r.primitive_states=ps;r.primitive_state_count=1;
        r.acquisition_states=as;r.acquisition_state_count=3;
        const auto normalized=gp::evaluate(registry,r);
        req(normalized.route==ref::kPrimitiveA &&
            normalized.lifecycle==gp::LifecycleAction::NONE &&
            !normalized.acquisition &&
            normalized.threshold_units==17,
            "generic tertiary diagnostic threshold normalization");
    }
    std::cout<<"REFERENCE_EQUIVALENCE_CASES="<<cases<<"\n";
}

namespace synthetic {
constexpr reg::DomainId D{0xD3000001ull};
constexpr reg::ReuseMetricId M{0xD3000002ull};
constexpr reg::EvidenceSetId E{0xE3000001ull};
constexpr reg::EvidenceProfileId P{0x53000001ull};
constexpr reg::PrimitiveFamilyId F{0xF3000001ull};
constexpr reg::CapabilityId C{0x13000001ull};
constexpr reg::PrimitiveId A{0xA3000001ull};
constexpr reg::PrimitiveId B{0xB3000001ull};
constexpr reg::RepresentationId R{0xE3000148ull};
constexpr reg::AcquisitionPathId GATED{0xC3000001ull};
constexpr reg::AcquisitionPathId FAST{0xC3000002ull};
constexpr reg::AcquisitionPathId LOW{0xC3000003ull};
constexpr reg::AcquisitionPathId DISABLED{0xC3000004ull};
constexpr reg::ProvenanceRef PROV{"synthetic://policy-family","synthetic-policy-blob"};

constexpr reg::DomainDescriptor domains[]={{D,reg::ValidationState::VALIDATED,"SYNTH_DOMAIN",PROV}};
constexpr reg::ReuseMetricDescriptor metrics[]={{M,reg::ValidationState::VALIDATED,"SYNTH_REUSE",PROV}};
constexpr reg::EvidenceSetDescriptor evidence[]={{E,reg::ValidationState::VALIDATED,"SYNTH_EVIDENCE",PROV}};
constexpr reg::EvidenceProfileDescriptor profiles[]={{P,D,M,E,reg::ValidationState::VALIDATED,"SYNTH_PROFILE",PROV}};
constexpr reg::PrimitiveFamilyDescriptor families[]={{F,D,reg::ValidationState::VALIDATED,"SYNTH_FAMILY",PROV}};
constexpr reg::CapabilityDescriptor capabilities[]={{C,D,A,reg::ValidationState::VALIDATED,"SYNTH_CAP",PROV}};
constexpr reg::PrimitiveDescriptor primitives[]={
    {A,F,C,{},0,reg::PRIMITIVE_FALLBACK|reg::PRIMITIVE_NO_EXTRA_REPRESENTATION,reg::ValidationState::VALIDATED,"SYNTH_A",PROV},
    {B,F,C,R,4096,reg::PRIMITIVE_REQUIRES_RESIDENCY|reg::PRIMITIVE_REQUIRES_IDENTITY_VALIDATION,reg::ValidationState::VALIDATED,"SYNTH_B",PROV}
};
constexpr reg::AcquisitionPathDescriptor acquisitions[]={
    {GATED,B,0,reg::ValidationState::CAPABILITY_GATED,"SYNTH_GATED",PROV},
    {FAST,B,1,reg::ValidationState::VALIDATED,"SYNTH_FAST",PROV},
    {LOW,B,5,reg::ValidationState::VALIDATED,"SYNTH_LOW",PROV},
    {DISABLED,B,0,reg::ValidationState::DISABLED_BY_EVIDENCE,"SYNTH_DISABLED",PROV}
};
constexpr reg::AcquisitionThresholdDescriptor thresholds[]={
    {C,P,GATED,M,1,reg::ValidationState::VALIDATED,PROV},
    {C,P,FAST,M,10,reg::ValidationState::VALIDATED,PROV},
    {C,P,LOW,M,7,reg::ValidationState::VALIDATED,PROV},
    {C,P,DISABLED,M,1,reg::ValidationState::VALIDATED,PROV}
};
constexpr reg::ResidentPreferenceDescriptor prefs[]={{C,P,B,A,reg::ValidationState::VALIDATED,PROV}};
constexpr reg::LifecycleDescriptor life[]={{B,true,true,true,true,true,true,PROV}};
constexpr reg::RegistrationBundle bundle{
    "SYNTHETIC_POLICY_FAMILY",
    domains,1,metrics,1,evidence,1,profiles,1,families,1,capabilities,1,
    primitives,2,acquisitions,4,thresholds,4,prefs,1,life,1
};
}

static gp::PolicyRequest synthetic_request(
    std::uint64_t h,
    gp::PrimitiveRuntimeState* ps,
    gp::AcquisitionRuntimeState* as){
    gp::PolicyRequest r{};
    r.capability=synthetic::C;r.profile=synthetic::P;
    r.current_request_in_validated_domain=true;r.model_loaded=true;
    r.future_reuse_known=true;r.future_reuse_units=h;r.acquisition_allowed=true;
    r.primitive_states=ps;r.primitive_state_count=1;
    r.acquisition_states=as;r.acquisition_state_count=4;
    return r;
}

static void synthetic_policy_proof(){
    reg::PrimitiveRegistry registry;reg::RegistryError err{};
    req(registry.add_bundle(synthetic::bundle,&err)==reg::RegistryStatus::OK,"synthetic registration");

    gp::PrimitiveRuntimeState ps[]={{synthetic::B,false,false,true,true}};
    gp::AcquisitionRuntimeState as[]={
        {synthetic::GATED,true,false},
        {synthetic::FAST,true,false},
        {synthetic::LOW,true,false},
        {synthetic::DISABLED,true,false}
    };

    auto r=synthetic_request(1,ps,as);
    auto d=gp::evaluate(registry,r);
    req(d.route==synthetic::A&&d.lifecycle==gp::LifecycleAction::NONE,"gated/disabled paths must not activate");

    r.future_reuse_units=7;
    d=gp::evaluate(registry,r);
    req(d.route==synthetic::B&&d.lifecycle==gp::LifecycleAction::ACQUIRE&&d.acquisition==synthetic::LOW&&d.threshold_units==7,"eligible lower-priority path when higher-priority threshold unmet");

    r.future_reuse_units=10;
    d=gp::evaluate(registry,r);
    req(d.acquisition==synthetic::FAST&&d.threshold_units==10,"priority among eligible paths");
    req(d.requested_residency_bytes==4096&&d.require_post_acquisition_identity_validation,"synthetic acquisition contract");

    ps[0]={synthetic::B,true,true,true,true};
    r=synthetic_request(1,ps,as);
    d=gp::evaluate(registry,r);
    req(d.route==synthetic::B&&d.lifecycle==gp::LifecycleAction::NONE,"synthetic resident hysteresis");

    r.current_request_in_validated_domain=false;
    d=gp::evaluate(registry,r);
    req(d.route==synthetic::A&&d.lifecycle==gp::LifecycleAction::NONE&&d.preserve_existing_representation,"synthetic outside-domain preserve");

    r.current_request_in_validated_domain=true;ps[0].execution_available=false;
    d=gp::evaluate(registry,r);
    req(d.route==synthetic::A&&d.lifecycle==gp::LifecycleAction::EVICT,"execution unavailable eviction");

    ps[0].execution_available=true;r.model_loaded=false;
    d=gp::evaluate(registry,r);
    req(d.lifecycle==gp::LifecycleAction::EVICT,"model unload eviction");

    r.model_loaded=true;ps[0].residency_lease_granted=false;
    d=gp::evaluate(registry,r);
    req(d.lifecycle==gp::LifecycleAction::EVICT,"lease revoke eviction");

    ps[0].residency_lease_granted=true;r.future_reuse_units=0;
    d=gp::evaluate(registry,r);
    req(d.lifecycle==gp::LifecycleAction::EVICT,"zero reuse eviction");

    gp::PrimitiveRuntimeState dup_ps[]={
        {synthetic::B,false,false,true,true},
        {synthetic::B,false,false,true,true}
    };
    r=synthetic_request(10,dup_ps,as);r.primitive_state_count=2;
    d=gp::evaluate(registry,r);
    req(d.status==gp::PolicyStatus::INVALID_RUNTIME_STATE,"duplicate runtime state rejected");

    // A profile from another valid domain must not be silently used with this capability.
    reg::PrimitiveRegistry mixed;req(mixed.add_bundle(ref::q4k_down_reference_bundle(),&err)==reg::RegistryStatus::OK,"mixed reference");
    req(mixed.add_bundle(synthetic::bundle,&err)==reg::RegistryStatus::OK,"mixed synthetic");
    r=synthetic_request(10,ps,as);r.profile=ref::kProfile0;
    d=gp::evaluate(mixed,r);
    req(d.status==gp::PolicyStatus::PROFILE_DOMAIN_MISMATCH&&d.route==synthetic::A,"profile-domain mismatch fail closed");

    std::cout<<"SYNTHETIC_POLICY_FAMILY=PASS\n";
}

int main(){
    reference_equivalence();
    synthetic_policy_proof();
    std::cout<<"PHASE2_GENERIC_POLICY_ENGINE_ZERO_SCIENCE_QA=PASS\n";
    std::cout<<"NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING. NO PLACEMENT BENCHMARK.\n";
    return 0;
}
