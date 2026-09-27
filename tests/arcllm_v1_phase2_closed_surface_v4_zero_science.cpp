#include "../include/arcllm/v1/generic_policy_engine_v4.h"
#include "../include/arcllm/v1/generic_backend_binding_v4.h"
#include "../src/registrations/arcllm_v1_q4k_down_reference_registration_v2.h"
#include "../src/registrations/arcllm_v1_p8_segmented_reference_registration_v2.h"
#include "../src/arcllm_v1_b1_policy_runtime_hook.h"
#include <array>
#include <cstdlib>
#include <iostream>

using namespace arcllm::v1;
namespace gp = arcllm::v1::policy_v4;
namespace bind = arcllm::v1::binding_v4;
namespace reg = arcllm::v1::registry_v2;
namespace ref = arcllm::v1::reference_registration_v2;
namespace p8 = arcllm::v1::p8_registration_v2;

static void req(bool c,const char*m){if(!c){std::cerr<<"V4_QA_FAIL: "<<m<<"\n";std::exit(2);}}

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
static reg::PrimitiveId map_route(b1::Route x){return x==b1::Route::B_EXEC148?ref::kPrimitiveB:ref::kPrimitiveA;}

namespace holdout {
constexpr reg::DomainId I2D{0xD2002001ull}; constexpr reg::EvidenceSetId I2E{0xE2002001ull};
constexpr reg::EvidenceProfileId I2P{0x52002001ull}; constexpr reg::PrimitiveFamilyId I2F{0xF2002001ull};
constexpr reg::CapabilityId I2C{0x12002001ull}; constexpr reg::PrimitiveId I2BASE{0xA2002001ull}; constexpr reg::PrimitiveId I2DIRECT{0xB2002001ull};
constexpr reg::DomainId AND{0xDA642001ull}; constexpr reg::EvidenceSetId ANE{0xEA642001ull};
constexpr reg::EvidenceProfileId ANP{0x5A642001ull}; constexpr reg::PrimitiveFamilyId ANF{0xFA642001ull};
constexpr reg::CapabilityId ANC{0x1A642001ull}; constexpr reg::PrimitiveId ANSAFE{0xAA642001ull}; constexpr reg::PrimitiveId ANFAST{0xBA642001ull};
constexpr reg::ProvenanceRef I2PROV{"artifacts/ARCLLM_V1/ARCLLM_V1_I002_FINAL_ADJUDICATION_v0.1.json","closed-i002"};
constexpr reg::ProvenanceRef ANPROV{"artifacts/ANL64/ANL64_P7_FINAL_PROGRAM_ADJUDICATION_v0.1.json","79df1758f23f724890eb302907c0b2d3045df735"};
constexpr reg::DomainDescriptor i2d[]={{I2D,reg::ValidationState::VALIDATED,"I002_DIRECT_DOMAIN",I2PROV}};
constexpr reg::EvidenceSetDescriptor i2e[]={{I2E,reg::ValidationState::VALIDATED,"I002_CLOSED_EVIDENCE",I2PROV}};
constexpr reg::EvidenceProfileDescriptor i2p[]={{I2P,I2D,{},I2E,reg::ValidationState::VALIDATED,"I002_PROFILE",I2PROV}};
constexpr reg::PrimitiveFamilyDescriptor i2f[]={{I2F,I2D,reg::ValidationState::VALIDATED,"I002_DIRECT_FAMILY",I2PROV}};
constexpr reg::CapabilityDescriptor i2c[]={{I2C,I2D,I2DIRECT,I2BASE,reg::ValidationState::VALIDATED,"I002_CAP",I2PROV}};
constexpr reg::PrimitiveDescriptor i2prim[]={
 {I2BASE,I2F,I2C,{},0,reg::PRIMITIVE_FALLBACK|reg::PRIMITIVE_NO_EXTRA_REPRESENTATION,reg::ValidationState::VALIDATED,"I002_BASELINE",I2PROV},
 {I2DIRECT,I2F,I2C,{},0,reg::PRIMITIVE_NO_EXTRA_REPRESENTATION,reg::ValidationState::VALIDATED,"I002_DIRECT",I2PROV}
};
constexpr reg::RegistrationBundle i2bundle{"I002_V4_HOLDOUT",i2d,1,nullptr,0,i2e,1,i2p,1,i2f,1,i2c,1,i2prim,2,nullptr,0,nullptr,0,nullptr,0};

constexpr reg::DomainDescriptor ands[]={{AND,reg::ValidationState::VALIDATED,"ANL64_PLAN_BOUND_DOMAIN",ANPROV}};
constexpr reg::EvidenceSetDescriptor ane[]={{ANE,reg::ValidationState::VALIDATED,"ANL64_CLOSED_EVIDENCE",ANPROV}};
constexpr reg::EvidenceProfileDescriptor anp[]={{ANP,AND,{},ANE,reg::ValidationState::VALIDATED,"ANL64_PROFILE",ANPROV}};
constexpr reg::PrimitiveFamilyDescriptor anf[]={{ANF,AND,reg::ValidationState::VALIDATED,"ANL64_PLAN_BOUND_FAMILY",ANPROV}};
constexpr reg::CapabilityDescriptor anc[]={{ANC,AND,ANFAST,ANSAFE,reg::ValidationState::VALIDATED,"ANL64_CAP",ANPROV}};
constexpr reg::PrimitiveDescriptor anprim[]={
 {ANSAFE,ANF,ANC,{},0,reg::PRIMITIVE_FALLBACK|reg::PRIMITIVE_NO_EXTRA_REPRESENTATION,reg::ValidationState::VALIDATED,"ANL64_SAFE",ANPROV},
 {ANFAST,ANF,ANC,{},0,reg::PRIMITIVE_NO_EXTRA_REPRESENTATION,reg::ValidationState::VALIDATED,"ANL64_Q4_FAST",ANPROV}
};
constexpr reg::RegistrationBundle anbundle{"ANL64_V4_HOLDOUT",ands,1,nullptr,0,ane,1,anp,1,anf,1,anc,1,anprim,2,nullptr,0,nullptr,0,nullptr,0};
}

static void family1_equivalence(){
    reg::PrimitiveRegistry registry; reg::RegistryError err{};
    req(registry.add_bundle(ref::q4k_down_reference_bundle(),&err)==reg::RegistryStatus::OK,"family1 registration");
    constexpr std::array<std::uint64_t,14> H{0,1,2,3,4,14,15,16,17,32,33,36,37,64};
    std::uint64_t cases=0;
    for(int wi=0;wi<2;++wi) for(auto h:H) for(std::uint32_t mask=0;mask<(1u<<12);++mask){
        b1::RuntimeSignals old{};
        old.workload=wi==0?b1::Workload::WS:b1::Workload::WC;
        old.current_request_in_validated_domain=(mask&(1u<<0))!=0; old.model_loaded=(mask&(1u<<1))!=0;
        old.b_resident=(mask&(1u<<2))!=0; old.b_identity_valid=(mask&(1u<<3))!=0;
        old.b_execution_available=(mask&(1u<<4))!=0; old.residency_lease_granted=(mask&(1u<<5))!=0;
        old.future_reuse_known=(mask&(1u<<6))!=0; old.future_reuse_tokens=h;
        old.acquisition_allowed=(mask&(1u<<7))!=0; old.p1_available=(mask&(1u<<8))!=0;
        old.p1_vetoed=(mask&(1u<<9))!=0; old.p3_cold_available=(mask&(1u<<10))!=0; old.p0_cpu_available=(mask&(1u<<11))!=0;
        const auto od=b1::make_runtime_plan(old);
        gp::PrimitiveRuntimeState ps[]={
            {ref::kPrimitiveA,false,true,true,true,true},
            {ref::kPrimitiveB,old.b_resident,old.b_identity_valid,old.b_execution_available,old.b_resident&&old.b_execution_available,old.residency_lease_granted}
        };
        gp::AcquisitionRuntimeState as[]={
            {ref::kAcquirePrimary,old.p1_available,old.p1_vetoed},{ref::kAcquireSecondary,old.p3_cold_available,false},
            {ref::kAcquireTertiary,old.p0_cpu_available,false},{ref::kAcquireDisabledWarm,true,false}
        };
        gp::PolicyRequest r{}; r.capability=ref::kCapability; r.profile=wi==0?ref::kProfile0:ref::kProfile1;
        r.request_within_capability_evidence_scope=old.current_request_in_validated_domain; r.model_loaded=old.model_loaded;
        r.future_reuse_known=old.future_reuse_known; r.future_reuse_units=h; r.acquisition_allowed=old.acquisition_allowed;
        r.primitive_states=ps; r.primitive_state_count=2; r.acquisition_states=as; r.acquisition_state_count=4;
        const auto nd=gp::evaluate(registry,r);
        req(nd.status==gp::PolicyStatus::OK,"family1 status");
        req(nd.route==map_route(od.decision.route),"family1 route");
        req(nd.lifecycle==map_lifecycle(od.decision.lifecycle),"family1 lifecycle");
        req(nd.acquisition==map_acquisition(od.decision.lifecycle),"family1 acquisition");
        if(od.decision.threshold_tokens) req(nd.threshold_units==od.decision.threshold_tokens,"family1 threshold");
        req(nd.preserve_existing_representation==od.decision.preserve_existing_b,"family1 preserve");
        ++cases;
    }
    req(cases==114688,"family1 case count");
    std::cout<<"FAMILY1_EQUIVALENCE_CASES="<<cases<<"\n";
}

static void frozen_oracles(){
    reg::PrimitiveRegistry registry; reg::RegistryError err{};
    req(registry.add_bundle(p8::p8_segmented_bundle(),&err)==reg::RegistryStatus::OK,"p8 registration");
    gp::PrimitiveRuntimeState p8ps[]={{p8::kSegmented,false,false,true,false,true}};
    gp::AcquisitionRuntimeState p8as[]={{p8::kBuildResidency,true,false}};
    gp::PolicyRequest p{}; p.capability=p8::kCapability;p.profile=p8::kProfile;p.model_loaded=true;p.request_within_capability_evidence_scope=true;
    p.primitive_states=p8ps;p.primitive_state_count=1;p.acquisition_states=p8as;p.acquisition_state_count=1;
    auto d=gp::evaluate(registry,p);
    req(d.lifecycle==gp::LifecycleAction::ACQUIRE&&d.route==p8::kSegmented,"p8 mandatory acquire");
    p8as[0].available=false; d=gp::evaluate(registry,p); req(d.status==gp::PolicyStatus::NOT_READY&&!d.route,"p8 unavailable");
    p8ps[0]={p8::kSegmented,true,true,true,true,true}; p8as[0].available=true; d=gp::evaluate(registry,p);
    req(d.status==gp::PolicyStatus::OK&&d.route==p8::kSegmented&&d.lifecycle==gp::LifecycleAction::NONE,"p8 resident ready");

    reg::PrimitiveRegistry i2; req(i2.add_bundle(holdout::i2bundle,&err)==reg::RegistryStatus::OK,"i002 registration");
    gp::PrimitiveRuntimeState i2ps[]={{holdout::I2BASE,false,true,true,true,true},{holdout::I2DIRECT,false,true,true,true,true}};
    gp::PolicyRequest q{};q.capability=holdout::I2C;q.profile=holdout::I2P;q.model_loaded=true;q.request_within_capability_evidence_scope=true;q.primitive_states=i2ps;q.primitive_state_count=2;
    d=gp::evaluate(i2,q);req(d.route==holdout::I2DIRECT&&d.lifecycle==gp::LifecycleAction::NONE,"i002 direct");
    i2ps[1].execution_ready=false;d=gp::evaluate(i2,q);req(d.route==holdout::I2BASE,"i002 fallback");
    i2ps[0].execution_ready=false;d=gp::evaluate(i2,q);req(d.status==gp::PolicyStatus::NOT_READY&&!d.route,"i002 no-ready-route");

    reg::PrimitiveRegistry an;req(an.add_bundle(holdout::anbundle,&err)==reg::RegistryStatus::OK,"anl64 registration");
    gp::PrimitiveRuntimeState anps[]={{holdout::ANSAFE,false,true,true,true,true},{holdout::ANFAST,false,true,true,true,true}};
    gp::PolicyRequest a{};a.capability=holdout::ANC;a.profile=holdout::ANP;a.model_loaded=true;a.request_within_capability_evidence_scope=true;a.primitive_states=anps;a.primitive_state_count=2;
    d=gp::evaluate(an,a);req(d.route==holdout::ANFAST,"anl64 preferred");
    anps[1].execution_ready=false;d=gp::evaluate(an,a);req(d.route==holdout::ANSAFE,"anl64 safe fallback");
    anps[0].execution_ready=false;d=gp::evaluate(an,a);req(d.status==gp::PolicyStatus::NOT_READY&&!d.route,"anl64 no-ready-route");
    std::cout<<"FROZEN_ORACLES=P8,I002,ANL64_PASS\n";
}

class RecordingBackend final: public bind::BackendAdapter {
public:
    bool fail_resolve=false; bool fail_acquire=false; bool fail_validate=false; bool fail_release=false;
    std::uint64_t next_rep=100; int resolves=0,acquires=0,validates=0,releases=0;
    bind::BackendStatus acquire(reg::AcquisitionPathId,reg::PrimitiveId,std::uint64_t,bind::RepresentationHandle& out) noexcept override {
        ++acquires;if(fail_acquire)return bind::BackendStatus::ACQUISITION_FAILED;out={next_rep++};return bind::BackendStatus::OK;
    }
    bind::BackendStatus validate(bind::RepresentationHandle,reg::CapabilityId,reg::PrimitiveId) noexcept override {
        ++validates;return fail_validate?bind::BackendStatus::VALIDATION_FAILED:bind::BackendStatus::OK;
    }
    bind::BackendStatus release(reg::PrimitiveId,bind::RepresentationHandle) noexcept override {
        ++releases;return fail_release?bind::BackendStatus::RELEASE_FAILED:bind::BackendStatus::OK;
    }
    bind::BackendStatus resolve_primitive(reg::PrimitiveId p,bind::PrimitiveHandle& out) noexcept override {
        ++resolves;if(fail_resolve)return bind::BackendStatus::PRIMITIVE_UNAVAILABLE;out={p.value};return bind::BackendStatus::OK;
    }
};

static void binding_contract(){
    reg::PrimitiveRegistry registry;reg::RegistryError err{};req(registry.add_bundle(ref::q4k_down_reference_bundle(),&err)==reg::RegistryStatus::OK,"bind registration");
    RecordingBackend backend;bind::BindingState state{};
    gp::PolicyDecision direct{};direct.route=ref::kPrimitiveA;
    auto ar=bind::apply_decision(registry,ref::kCapability,direct,backend,state);
    req(ar.ready&&ar.primitive.opaque==ref::kPrimitiveA.value&&backend.resolves==1,"bind direct route");
    backend.fail_resolve=true;ar=bind::apply_decision(registry,ref::kCapability,direct,backend,state);
    req(!ar.ready&&backend.resolves==2,"no hidden fallback on resolve failure");
    backend.fail_resolve=false;

    gp::PolicyDecision acq{};acq.route=ref::kPrimitiveB;acq.lifecycle=gp::LifecycleAction::ACQUIRE;acq.acquisition=ref::kAcquirePrimary;
    acq.requested_residency_bytes=549527552ull;acq.require_post_acquisition_identity_validation=true;
    ar=bind::apply_decision(registry,ref::kCapability,acq,backend,state);
    req(ar.ready&&backend.acquires==1&&backend.validates==1&&state.find(ref::kPrimitiveB),"bind acquire");
    gp::PolicyDecision ev{};ev.route=ref::kPrimitiveA;ev.lifecycle=gp::LifecycleAction::EVICT;
    ar=bind::apply_decision(registry,ref::kCapability,ev,backend,state);
    req(ar.ready&&backend.releases==1&&!state.find(ref::kPrimitiveB),"bind evict");

    backend.fail_acquire=true;ar=bind::apply_decision(registry,ref::kCapability,acq,backend,state);
    req(!ar.ready&&backend.resolves==4,"acquisition failure does not silently fallback");
    std::cout<<"GENERIC_BACKEND_BINDING_CONTRACT=PASS\n";
}

int main(){
    family1_equivalence();
    frozen_oracles();
    binding_contract();
    std::cout<<"PHASE2_CLOSED_SURFACE_V4_MATERIALIZATION_QA=PASS\n";
    std::cout<<"NO_MODEL_LOAD_NO_GPU_NO_VULKAN_NO_TIMING\n";
    return 0;
}
