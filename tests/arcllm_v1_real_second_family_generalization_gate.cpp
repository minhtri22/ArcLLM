#include "../include/arcllm/v1/generic_policy_engine.h"
#include <cstdlib>
#include <iostream>

namespace reg=arcllm::v1::registry;
namespace gp=arcllm::v1::policy;

static void req(bool c,const char*m){if(!c){std::cerr<<"REAL_SECOND_FAMILY_GATE_HARNESS_FAIL: "<<m<<"\n";std::exit(2);}}

namespace faithful {
constexpr reg::DomainId D{0xD8000001ull};
constexpr reg::ReuseMetricId M{0xD8000002ull};
constexpr reg::EvidenceSetId E{0xE8000001ull};
constexpr reg::EvidenceProfileId P{0x58000001ull};
constexpr reg::PrimitiveFamilyId F{0xF8000001ull};
constexpr reg::CapabilityId C{0x18000001ull};
constexpr reg::PrimitiveId CONTIG{0xA8000001ull};
constexpr reg::PrimitiveId SEG{0xB8000001ull};
constexpr reg::RepresentationId R{0xE8000148ull};
constexpr reg::AcquisitionPathId BUILD{0xC8000001ull};
constexpr reg::ProvenanceRef PROV{"P8 frozen evidence chain","p8-evidence"};

constexpr reg::DomainDescriptor domains[]={{D,reg::ValidationState::VALIDATED,"P8_BOUNDED_SEGMENTED_GRAPH_DOMAIN",PROV}};
constexpr reg::ReuseMetricDescriptor metrics[]={{M,reg::ValidationState::VALIDATED,"STRUCTURALLY_REQUIRED_UNUSED_REUSE_FIELD",PROV}};
constexpr reg::EvidenceSetDescriptor evidence[]={{E,reg::ValidationState::VALIDATED,"P8_A_TO_G6_FROZEN_EVIDENCE",PROV}};
constexpr reg::EvidenceProfileDescriptor profiles[]={{P,D,M,E,reg::ValidationState::VALIDATED,"P8_BOUNDED_PROFILE",PROV}};
constexpr reg::PrimitiveFamilyDescriptor families[]={{F,D,reg::ValidationState::VALIDATED,"P8_SEGMENTED_FAMILY",PROV}};
constexpr reg::CapabilityDescriptor caps[]={{C,D,CONTIG,reg::ValidationState::VALIDATED,"P8_BOUNDED_SEGMENTED_CAPABILITY",PROV}};
constexpr reg::PrimitiveDescriptor primitives[]={
 {CONTIG,F,C,{},0,reg::PRIMITIVE_FALLBACK|reg::PRIMITIVE_NO_EXTRA_REPRESENTATION,reg::ValidationState::DISABLED_BY_EVIDENCE,"P8_CONTIGUOUS_FAILED",PROV},
 {SEG,F,C,R,5347770372ull,reg::PRIMITIVE_REQUIRES_RESIDENCY|reg::PRIMITIVE_REQUIRES_IDENTITY_VALIDATION,reg::ValidationState::VALIDATED,"P8_SEGMENTED_VALIDATED",PROV}
};
constexpr reg::AcquisitionPathDescriptor acquisitions[]={{BUILD,SEG,0,reg::ValidationState::VALIDATED,"P8_ALLOCATE_COPY_SEGMENTED_RESIDENCY",PROV}};
constexpr reg::AcquisitionThresholdDescriptor thresholds[]={{C,P,BUILD,M,0,reg::ValidationState::VALIDATED,PROV}};
constexpr reg::ResidentPreferenceDescriptor prefs[]={{C,P,SEG,CONTIG,reg::ValidationState::VALIDATED,PROV}};
constexpr reg::LifecycleDescriptor life[]={{SEG,true,true,true,true,true,true,PROV}};
constexpr reg::RegistrationBundle bundle{
 "P8_EVIDENCE_FAITHFUL_ENCODING",
 domains,1,metrics,1,evidence,1,profiles,1,families,1,caps,1,
 primitives,2,acquisitions,1,thresholds,1,prefs,1,life,1
};
}

namespace selffallback {
constexpr reg::DomainId D{0xD8100001ull};
constexpr reg::ReuseMetricId M{0xD8100002ull};
constexpr reg::EvidenceSetId E{0xE8100001ull};
constexpr reg::EvidenceProfileId P{0x58100001ull};
constexpr reg::PrimitiveFamilyId F{0xF8100001ull};
constexpr reg::CapabilityId C{0x18100001ull};
constexpr reg::PrimitiveId SEG{0xB8100001ull};
constexpr reg::RepresentationId R{0xE8100148ull};
constexpr reg::AcquisitionPathId BUILD{0xC8100001ull};
constexpr reg::ProvenanceRef PROV{"P8 frozen evidence chain","p8-evidence"};

constexpr reg::DomainDescriptor domains[]={{D,reg::ValidationState::VALIDATED,"P8_BOUNDED_SEGMENTED_GRAPH_DOMAIN",PROV}};
constexpr reg::ReuseMetricDescriptor metrics[]={{M,reg::ValidationState::VALIDATED,"STRUCTURALLY_REQUIRED_UNUSED_REUSE_FIELD",PROV}};
constexpr reg::EvidenceSetDescriptor evidence[]={{E,reg::ValidationState::VALIDATED,"P8_A_TO_G6_FROZEN_EVIDENCE",PROV}};
constexpr reg::EvidenceProfileDescriptor profiles[]={{P,D,M,E,reg::ValidationState::VALIDATED,"P8_BOUNDED_PROFILE",PROV}};
constexpr reg::PrimitiveFamilyDescriptor families[]={{F,D,reg::ValidationState::VALIDATED,"P8_SEGMENTED_FAMILY",PROV}};
constexpr reg::CapabilityDescriptor caps[]={{C,D,SEG,reg::ValidationState::VALIDATED,"P8_BOUNDED_SEGMENTED_CAPABILITY",PROV}};
constexpr reg::PrimitiveDescriptor primitives[]={
 {SEG,F,C,R,5347770372ull,reg::PRIMITIVE_FALLBACK|reg::PRIMITIVE_REQUIRES_RESIDENCY|reg::PRIMITIVE_REQUIRES_IDENTITY_VALIDATION,reg::ValidationState::VALIDATED,"P8_SEGMENTED_VALIDATED",PROV}
};
constexpr reg::AcquisitionPathDescriptor acquisitions[]={{BUILD,SEG,0,reg::ValidationState::VALIDATED,"P8_ALLOCATE_COPY_SEGMENTED_RESIDENCY",PROV}};
constexpr reg::AcquisitionThresholdDescriptor thresholds[]={{C,P,BUILD,M,0,reg::ValidationState::VALIDATED,PROV}};
constexpr reg::ResidentPreferenceDescriptor prefs[]={{C,P,SEG,SEG,reg::ValidationState::VALIDATED,PROV}};
constexpr reg::LifecycleDescriptor life[]={{SEG,true,true,true,true,true,true,PROV}};
constexpr reg::RegistrationBundle bundle{
 "P8_SELF_FALLBACK_ENCODING",
 domains,1,metrics,1,evidence,1,profiles,1,families,1,caps,1,
 primitives,1,acquisitions,1,thresholds,1,prefs,1,life,1
};
}

static gp::PolicyRequest request(
 reg::CapabilityId c,reg::EvidenceProfileId p,reg::PrimitiveId seg,reg::AcquisitionPathId build,
 bool resident,bool reuse_known,std::uint64_t reuse_units){
 static gp::PrimitiveRuntimeState ps[1];
 static gp::AcquisitionRuntimeState as[1];
 ps[0]={seg,resident,resident,true,true};
 as[0]={build,true,false};
 gp::PolicyRequest r{};
 r.capability=c;r.profile=p;
 r.current_request_in_validated_domain=true;
 r.model_loaded=true;
 r.future_reuse_known=reuse_known;
 r.future_reuse_units=reuse_units;
 r.acquisition_allowed=true;
 r.primitive_states=ps;r.primitive_state_count=1;
 r.acquisition_states=as;r.acquisition_state_count=1;
 return r;
}

int main(){
 {
  reg::PrimitiveRegistry registry;reg::RegistryError e{};
  req(registry.add_bundle(faithful::bundle,&e)==reg::RegistryStatus::OK,"faithful bundle must be structurally registerable");
  auto r=request(faithful::C,faithful::P,faithful::SEG,faithful::BUILD,false,false,0);
  const auto d=gp::evaluate(registry,r);
  req(d.status==gp::PolicyStatus::REGISTRY_POLICY_INCOMPLETE,
      "evidence-faithful disabled fallback must expose frozen active-fallback assumption");
  std::cout<<"COUNTEREXAMPLE_1=ACTIVE_VALIDATED_FALLBACK_REQUIRED\n";
 }
 {
  reg::PrimitiveRegistry registry;reg::RegistryError e{};
  req(registry.add_bundle(selffallback::bundle,&e)==reg::RegistryStatus::OK,"self-fallback bundle register");
  auto unknown=request(selffallback::C,selffallback::P,selffallback::SEG,selffallback::BUILD,false,false,0);
  const auto du=gp::evaluate(registry,unknown);
  req(du.status==gp::PolicyStatus::OK,"self fallback unknown reuse status");
  req(du.route==selffallback::SEG && du.lifecycle==gp::LifecycleAction::NONE && !du.acquisition,
      "reuse-unknown must expose route-without-acquisition mismatch");

  auto zero=request(selffallback::C,selffallback::P,selffallback::SEG,selffallback::BUILD,false,true,0);
  const auto dz=gp::evaluate(registry,zero);
  req(dz.route==selffallback::SEG && dz.lifecycle==gp::LifecycleAction::NONE && !dz.acquisition,
      "zero-reuse must expose route-without-acquisition mismatch");

  auto invented=request(selffallback::C,selffallback::P,selffallback::SEG,selffallback::BUILD,false,true,1);
  const auto di=gp::evaluate(registry,invented);
  req(di.lifecycle==gp::LifecycleAction::ACQUIRE && di.acquisition==selffallback::BUILD,
      "positive invented reuse shows acquisition is reachable only after adding unsupported reuse semantics");

  std::cout<<"COUNTEREXAMPLE_2=ACQUISITION_IS_HARDCODED_BEHIND_REUSE_SEMANTICS\n";
 }
 std::cout<<"PHASE2_REAL_SECOND_FAMILY_GENERALIZATION_GATE=FAIL_ABSTRACTION_INCOMPLETE\n";
 std::cout<<"REQUIRED_REDESIGN_CLASS=MANDATORY_FEASIBILITY_CAPABILITY_WITH_OPTIONAL_FALLBACK_AND_NON_REUSE_ACQUISITION_TRIGGER\n";
 std::cout<<"FROZEN_GENERIC_CORE_FILES_MODIFIED=NO\n";
 std::cout<<"NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING. NO PLACEMENT BENCHMARK.\n";
 return 0;
}
