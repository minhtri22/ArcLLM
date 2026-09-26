#include "../include/arcllm/v1/generic_policy_engine.h"
#include "../include/arcllm/v1/generic_policy_engine_v2.h"
#include "../src/registrations/arcllm_v1_q4k_down_reference_registration.h"
#include "../src/registrations/arcllm_v1_q4k_down_reference_registration_v2.h"
#include "../src/registrations/arcllm_v1_p8_segmented_reference_registration_v2.h"
#include <array>
#include <cstdlib>
#include <iostream>

namespace r1=arcllm::v1::registry;
namespace p1=arcllm::v1::policy;
namespace rr1=arcllm::v1::reference_registration;
namespace r2=arcllm::v1::registry_v2;
namespace p2=arcllm::v1::policy_v2;
namespace rr2=arcllm::v1::reference_registration_v2;
namespace p8=arcllm::v1::p8_registration_v2;

static void req(bool c,const char*m){if(!c){std::cerr<<"CAPABILITY_REDESIGN_FAIL: "<<m<<"\n";std::exit(2);}}

static void first_family_equivalence(){
    r1::PrimitiveRegistry old_registry;r1::RegistryError e1{};
    r2::PrimitiveRegistry new_registry;r2::RegistryError e2{};
    req(old_registry.add_bundle(rr1::q4k_down_reference_bundle(),&e1)==r1::RegistryStatus::OK,"old reference registration");
    req(new_registry.add_bundle(rr2::q4k_down_reference_bundle(),&e2)==r2::RegistryStatus::OK,"v2 reference registration");

    constexpr std::array<std::uint64_t,14> H{0,1,2,3,4,14,15,16,17,32,33,36,37,64};
    std::uint64_t cases=0;
    for(int wi=0;wi<2;++wi){
        for(auto h:H){
            for(std::uint32_t mask=0;mask<(1u<<12);++mask){
                const bool in_domain=(mask&(1u<<0))!=0;
                const bool model=(mask&(1u<<1))!=0;
                const bool resident=(mask&(1u<<2))!=0;
                const bool identity=(mask&(1u<<3))!=0;
                const bool execution=(mask&(1u<<4))!=0;
                const bool lease=(mask&(1u<<5))!=0;
                const bool reuse_known=(mask&(1u<<6))!=0;
                const bool acquisition_allowed=(mask&(1u<<7))!=0;
                const bool p1_available=(mask&(1u<<8))!=0;
                const bool p1_vetoed=(mask&(1u<<9))!=0;
                const bool p3_available=(mask&(1u<<10))!=0;
                const bool p0_available=(mask&(1u<<11))!=0;

                p1::PrimitiveRuntimeState ops[]={{rr1::kPrimitiveB,resident,identity,execution,lease}};
                p1::AcquisitionRuntimeState oas[]={
                    {rr1::kAcquirePrimary,p1_available,p1_vetoed},
                    {rr1::kAcquireSecondary,p3_available,false},
                    {rr1::kAcquireTertiary,p0_available,false},
                    {rr1::kAcquireDisabledWarm,true,false}
                };
                p1::PolicyRequest orq{};
                orq.capability=rr1::kCapability;
                orq.profile=wi==0?rr1::kProfile0:rr1::kProfile1;
                orq.current_request_in_validated_domain=in_domain;
                orq.model_loaded=model;
                orq.future_reuse_known=reuse_known;
                orq.future_reuse_units=h;
                orq.acquisition_allowed=acquisition_allowed;
                orq.primitive_states=ops;orq.primitive_state_count=1;
                orq.acquisition_states=oas;orq.acquisition_state_count=4;
                const auto od=p1::evaluate(old_registry,orq);

                p2::PrimitiveRuntimeState nps[]={{rr2::kPrimitiveB,resident,identity,execution,lease}};
                p2::AcquisitionRuntimeState nas[]={
                    {rr2::kAcquirePrimary,p1_available,p1_vetoed},
                    {rr2::kAcquireSecondary,p3_available,false},
                    {rr2::kAcquireTertiary,p0_available,false},
                    {rr2::kAcquireDisabledWarm,true,false}
                };
                p2::PolicyRequest nrq{};
                nrq.capability=rr2::kCapability;
                nrq.profile=wi==0?rr2::kProfile0:rr2::kProfile1;
                nrq.request_within_capability_evidence_scope=in_domain;
                nrq.model_loaded=model;
                nrq.future_reuse_known=reuse_known;
                nrq.future_reuse_units=h;
                nrq.acquisition_allowed=acquisition_allowed;
                nrq.primitive_states=nps;nrq.primitive_state_count=1;
                nrq.acquisition_states=nas;nrq.acquisition_state_count=4;
                const auto nd=p2::evaluate(new_registry,nrq);

                req(static_cast<int>(nd.status)==static_cast<int>(od.status),"first-family status");
                req(nd.route.value==od.route.value,"first-family route");
                req(static_cast<int>(nd.lifecycle)==static_cast<int>(od.lifecycle),"first-family lifecycle");
                req(nd.acquisition.value==od.acquisition.value,"first-family acquisition");
                req(nd.threshold_units==od.threshold_units,"first-family threshold");
                req(nd.requested_residency_bytes==od.requested_residency_bytes,"first-family residency bytes");
                req(nd.preserve_existing_representation==od.preserve_existing_representation,"first-family preserve");
                req(nd.require_post_acquisition_identity_validation==od.require_post_acquisition_identity_validation,"first-family identity validation");
                ++cases;
            }
        }
    }
    req(cases==114688,"first-family case count");
    std::cout<<"FIRST_FAMILY_EQUIVALENCE_CASES="<<cases<<"\n";
}

static p2::PolicyRequest p8_request(
    p2::PrimitiveRuntimeState* ps,
    p2::AcquisitionRuntimeState* as,
    bool in_scope=true,
    bool model_loaded=true,
    bool reuse_known=false,
    std::uint64_t reuse=0,
    bool acquisition_allowed=true){
    p2::PolicyRequest r{};
    r.capability=p8::kCapability;
    r.profile=p8::kProfile;
    r.request_within_capability_evidence_scope=in_scope;
    r.model_loaded=model_loaded;
    r.future_reuse_known=reuse_known;
    r.future_reuse_units=reuse;
    r.acquisition_allowed=acquisition_allowed;
    r.primitive_states=ps;r.primitive_state_count=1;
    r.acquisition_states=as;r.acquisition_state_count=1;
    return r;
}

static void p8_oracle(){
    r2::PrimitiveRegistry registry;r2::RegistryError e{};
    req(registry.add_bundle(p8::p8_segmented_bundle(),&e)==r2::RegistryStatus::OK,"P8 registration");
    const auto*profile=registry.find_profile(p8::kProfile);
    req(profile&&!profile->reuse_metric,"P8 must have no invented reuse metric");
    req(registry.bundle_at(0)->threshold_count==0,"P8 mandatory acquisition must have no threshold");
    req(registry.find_acquisition(p8::kBuildResidency)->trigger==r2::AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY,"P8 trigger");

    p2::PrimitiveRuntimeState ps[]={{p8::kSegmented,false,false,true,true}};
    p2::AcquisitionRuntimeState as[]={{p8::kBuildResidency,true,false}};

    auto r=p8_request(ps,as,true,true,false,0,true);
    auto d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::OK&&d.route==p8::kSegmented&&d.lifecycle==p2::LifecycleAction::ACQUIRE&&d.acquisition==p8::kBuildResidency,"P8 unknown-reuse mandatory acquire");
    req(d.threshold_units==0&&d.requested_residency_bytes==5347770372ull&&d.require_post_acquisition_identity_validation,"P8 acquisition contract");

    r=p8_request(ps,as,true,true,true,0,true);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::OK&&d.lifecycle==p2::LifecycleAction::ACQUIRE&&d.acquisition==p8::kBuildResidency,"P8 zero-reuse mandatory acquire");

    as[0].available=false;
    r=p8_request(ps,as);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::NOT_READY&&!d.route&&d.lifecycle==p2::LifecycleAction::NONE&&!d.acquisition,"P8 unavailable is first-class not-ready");

    as[0].available=true;ps[0].residency_lease_granted=false;
    r=p8_request(ps,as);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::NOT_READY&&!d.route&&!d.acquisition,"P8 lease denied not-ready");

    ps[0]={p8::kSegmented,true,true,true,true};
    r=p8_request(ps,as,true,true,false,0,true);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::OK&&d.route==p8::kSegmented&&d.lifecycle==p2::LifecycleAction::NONE,"P8 resident use");

    r=p8_request(ps,as,true,true,true,0,true);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::OK&&d.route==p8::kSegmented&&d.lifecycle==p2::LifecycleAction::NONE,"P8 zero reuse must not invent eviction semantics");

    r=p8_request(ps,as,false,true,false,0,true);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::OUTSIDE_VALIDATED_CAPABILITY&&!d.route&&d.lifecycle==p2::LifecycleAction::NONE&&d.preserve_existing_representation,"P8 resident outside bounded evidence scope");

    ps[0]={p8::kSegmented,false,false,true,true};
    r=p8_request(ps,as,false,true,false,0,true);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::OUTSIDE_VALIDATED_CAPABILITY&&!d.route&&!d.acquisition,"P8 absent outside bounded evidence scope");

    ps[0]={p8::kSegmented,true,true,false,true};
    r=p8_request(ps,as,true,true,false,0,true);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::NOT_READY&&!d.route&&d.lifecycle==p2::LifecycleAction::EVICT,"P8 invalid resident evicts without fake fallback");

    ps[0]={p8::kSegmented,true,true,true,true};
    r=p8_request(ps,as,true,false,false,0,true);
    d=p2::evaluate(registry,r);
    req(d.status==p2::PolicyStatus::NOT_READY&&!d.route&&d.lifecycle==p2::LifecycleAction::EVICT,"P8 model unload eviction");

    std::cout<<"P8_REAL_FAMILY_ORACLE=PASS\n";
}

static void structural_semantics_guards(){
    using namespace r2;
    constexpr DomainId D{0xD9000001ull};
    constexpr EvidenceSetId E{0xE9000001ull};
    constexpr EvidenceProfileId P{0x59000001ull};
    constexpr PrimitiveFamilyId F{0xF9000001ull};
    constexpr CapabilityId C{0x19000001ull};
    constexpr PrimitiveId X{0xB9000001ull};
    constexpr RepresentationId R{0xE9000001ull};
    constexpr AcquisitionPathId A{0xC9000001ull};
    constexpr ReuseMetricId M{0xD9000002ull};
    constexpr ProvenanceRef V{"guard","guard"};
    static constexpr DomainDescriptor ds[]={{D,ValidationState::VALIDATED,"D",V}};
    static constexpr ReuseMetricDescriptor ms[]={{M,ValidationState::VALIDATED,"M",V}};
    static constexpr EvidenceSetDescriptor es[]={{E,ValidationState::VALIDATED,"E",V}};
    static constexpr EvidenceProfileDescriptor ps[]={{P,D,{},E,ValidationState::VALIDATED,"P",V}};
    static constexpr PrimitiveFamilyDescriptor fs[]={{F,D,ValidationState::VALIDATED,"F",V}};
    static constexpr CapabilityDescriptor cs[]={{C,D,X,{},ValidationState::VALIDATED,"C",V}};
    static constexpr PrimitiveDescriptor xs[]={{X,F,C,R,4096,PRIMITIVE_REQUIRES_RESIDENCY,ValidationState::VALIDATED,"X",V}};
    static constexpr AcquisitionPathDescriptor as[]={{A,X,0,AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY,ValidationState::VALIDATED,"A",V}};
    static constexpr AcquisitionThresholdDescriptor ts[]={{C,P,A,M,1,ValidationState::VALIDATED,V}};
    static constexpr RegistrationBundle bad{
        "MANDATORY_WITH_THRESHOLD_MUST_FAIL",
        ds,1,ms,1,es,1,ps,1,fs,1,cs,1,xs,1,as,1,ts,1,nullptr,0
    };
    PrimitiveRegistry registry;RegistryError e{};
    req(registry.add_bundle(bad,&e)==RegistryStatus::INVALID_ACQUISITION_SEMANTICS,"mandatory threshold rejection");
    req(registry.bundle_count()==0,"mandatory threshold rejection atomic");
    std::cout<<"SEMANTIC_GUARDS=PASS\n";
}

int main(){
    first_family_equivalence();
    p8_oracle();
    structural_semantics_guards();
    std::cout<<"PHASE2_CAPABILITY_ACQUISITION_SEMANTICS_REDESIGN_GATE=PASS\n";
    std::cout<<"NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING. NO NEW PLACEMENT STUDY.\n";
    return 0;
}
