#include "../include/arcllm/v1/primitive_registry_api.h"
#include "../src/registrations/arcllm_v1_q4k_down_reference_registration.h"
#include <cstdlib>
#include <iostream>

using namespace arcllm::v1::registry;
namespace ref = arcllm::v1::reference_registration;

static void req(bool c,const char*m){if(!c){std::cerr<<"REGISTRY_FAIL: "<<m<<"\n";std::exit(2);}}

namespace synthetic {
constexpr DomainId D{0xD1000001ull};
constexpr ReuseMetricId M{0xD1000002ull};
constexpr EvidenceSetId E{0xE1000001ull};
constexpr EvidenceProfileId P{0x51010001ull};
constexpr PrimitiveFamilyId F{0xF1000001ull};
constexpr CapabilityId C{0x11000001ull};
constexpr PrimitiveId A{0xA1000001ull};
constexpr PrimitiveId B{0xB1000001ull};
constexpr RepresentationId R{0xE1000148ull};
constexpr AcquisitionPathId X{0xC1000001ull};
constexpr ProvenanceRef PROV{"synthetic://family-two","synthetic-blob"};

constexpr DomainDescriptor domains[]={{D,ValidationState::VALIDATED,"SYNTH_DOMAIN",PROV}};
constexpr ReuseMetricDescriptor metrics[]={{M,ValidationState::VALIDATED,"SYNTH_REUSE",PROV}};
constexpr EvidenceSetDescriptor evidence[]={{E,ValidationState::VALIDATED,"SYNTH_EVIDENCE",PROV}};
constexpr EvidenceProfileDescriptor profiles[]={{P,D,M,E,ValidationState::VALIDATED,"SYNTH_PROFILE",PROV}};
constexpr PrimitiveFamilyDescriptor families[]={{F,D,ValidationState::VALIDATED,"SYNTH_FAMILY",PROV}};
constexpr CapabilityDescriptor capabilities[]={{C,D,A,ValidationState::VALIDATED,"SYNTH_CAP",PROV}};
constexpr PrimitiveDescriptor primitives[]={
    {A,F,C,{},0,PRIMITIVE_FALLBACK|PRIMITIVE_NO_EXTRA_REPRESENTATION,ValidationState::VALIDATED,"SYNTH_A",PROV},
    {B,F,C,R,4096,PRIMITIVE_REQUIRES_RESIDENCY|PRIMITIVE_REQUIRES_IDENTITY_VALIDATION,ValidationState::VALIDATED,"SYNTH_B",PROV}
};
constexpr AcquisitionPathDescriptor acquisitions[]={{X,B,0,ValidationState::VALIDATED,"SYNTH_X",PROV}};
constexpr AcquisitionThresholdDescriptor thresholds[]={{C,P,X,M,7,ValidationState::VALIDATED,PROV}};
constexpr ResidentPreferenceDescriptor prefs[]={{C,P,B,A,ValidationState::VALIDATED,PROV}};
constexpr LifecycleDescriptor life[]={{B,true,true,true,true,true,true,PROV}};
constexpr RegistrationBundle bundle{
    "SYNTHETIC_SECOND_FAMILY",
    domains,1,metrics,1,evidence,1,profiles,1,families,1,capabilities,1,
    primitives,2,acquisitions,1,thresholds,1,prefs,1,life,1
};
}

int main(){
    PrimitiveRegistry registry;
    RegistryError err{};

    req(registry.add_bundle(ref::q4k_down_reference_bundle(),&err)==RegistryStatus::OK,"reference bundle add");
    req(registry.bundle_count()==1,"reference bundle count");

    const auto* cap=registry.find_capability(ref::kCapability);
    req(cap&&cap->fallback_primitive==ref::kPrimitiveA,"reference fallback");
    const auto* a=registry.find_primitive(ref::kPrimitiveA);
    const auto* b=registry.find_primitive(ref::kPrimitiveB);
    req(a&&b,"reference primitives");
    req((a->flags&PRIMITIVE_NO_EXTRA_REPRESENTATION)!=0&&a->residency_bytes==0,"A descriptor");
    req((b->flags&PRIMITIVE_REQUIRES_RESIDENCY)!=0&&b->residency_bytes==549527552ull,"B descriptor");
    req(registry.find_acquisition(ref::kAcquireDisabledWarm)->state==ValidationState::DISABLED_BY_EVIDENCE,"P3 warm evidence-state retention");
    req(registry.find_threshold(ref::kCapability,ref::kProfile0,ref::kAcquirePrimary)->minimum_reuse_units==2,"profile0 primary threshold");
    req(registry.find_threshold(ref::kCapability,ref::kProfile1,ref::kAcquirePrimary)->minimum_reuse_units==4,"profile1 primary threshold");
    req(registry.find_threshold(ref::kCapability,ref::kProfile0,ref::kAcquireSecondary)->minimum_reuse_units==15,"profile0 secondary threshold");
    req(registry.find_threshold(ref::kCapability,ref::kProfile1,ref::kAcquireTertiary)->minimum_reuse_units==37,"profile1 tertiary threshold");
    const auto* pref=registry.find_resident_preference(ref::kCapability,ref::kProfile0);
    req(pref&&pref->preferred_when_resident==ref::kPrimitiveB&&pref->fallback_primitive==ref::kPrimitiveA,"resident preference");
    const auto* life=registry.find_lifecycle(ref::kPrimitiveB);
    req(life&&life->preserve_residency_outside_domain&&life->evict_on_lease_revoke&&life->evict_on_identity_invalid&&life->evict_on_execution_unavailable&&life->evict_on_model_unload&&life->evict_on_zero_future_reuse,"lifecycle");

    // Scalability proof: second family is data-only. Generic registry core/API is unchanged.
    req(registry.add_bundle(synthetic::bundle,&err)==RegistryStatus::OK,"synthetic family add");
    req(registry.bundle_count()==2,"synthetic bundle count");
    req(registry.find_capability(synthetic::C)!=nullptr,"synthetic capability lookup");
    req(registry.find_primitive(synthetic::B)->residency_bytes==4096,"synthetic primitive lookup");
    req(registry.find_threshold(synthetic::C,synthetic::P,synthetic::X)->minimum_reuse_units==7,"synthetic threshold lookup");

    const auto before=registry.bundle_count();
    req(registry.add_bundle(synthetic::bundle,&err)==RegistryStatus::DUPLICATE_ID,"duplicate family rejected");
    req(registry.bundle_count()==before,"duplicate rejection atomic");

    constexpr DomainId badD{0xD2000001ull};
    constexpr CapabilityId badC{0x12000001ull};
    constexpr PrimitiveId missing{0xA20000FFull};
    static constexpr DomainDescriptor badDomains[]={{badD,ValidationState::VALIDATED,"BAD_DOMAIN",synthetic::PROV}};
    static constexpr CapabilityDescriptor badCaps[]={{badC,badD,missing,ValidationState::VALIDATED,"BAD_CAP",synthetic::PROV}};
    static constexpr RegistrationBundle badBundle{
        "INVALID_REFERENCE_BUNDLE",
        badDomains,1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,badCaps,1,
        nullptr,0,nullptr,0,nullptr,0,nullptr,0,nullptr,0
    };
    req(registry.add_bundle(badBundle,&err)==RegistryStatus::INVALID_REFERENCE,"invalid reference rejected");
    req(registry.bundle_count()==before,"invalid rejection atomic");

    std::cout<<"PHASE2_GENERIC_PRIMITIVE_REGISTRY_ZERO_SCIENCE_QA=PASS\n";
    std::cout<<"REFERENCE_BUNDLES=1 SYNTHETIC_ADDITIONAL_FAMILIES=1\n";
    std::cout<<"NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING. NO PLACEMENT BENCHMARK.\n";
    return 0;
}
