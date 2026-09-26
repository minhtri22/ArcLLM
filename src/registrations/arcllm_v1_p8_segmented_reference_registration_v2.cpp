#include "arcllm_v1_p8_segmented_reference_registration_v2.h"

namespace arcllm::v1::p8_registration_v2 {
namespace {
using namespace registry_v2;

constexpr ProvenanceRef kP8{
 "artifacts/ARCLLM_V1/ARCLLM_V1_PHASE2_REAL_SECOND_FAMILY_GENERALIZATION_ADJUDICATION_v0.1.json",
 "a7430a4fd941b555b9262c464a3473bd3d7cf942"
};

constexpr DomainDescriptor kDomains[]={{kDomain,ValidationState::VALIDATED,"P8_BOUNDED_THROUGH_TWO_LAYER_CORRECTNESS",kP8}};
constexpr EvidenceSetDescriptor kEvidence[]={{kEvidenceSet,ValidationState::VALIDATED,"P8_A_FAIL_A2_TO_F_PASS_G_FAIL_BOUNDARY",kP8}};
constexpr EvidenceProfileDescriptor kProfiles[]={{kProfile,kDomain,{},kEvidenceSet,ValidationState::VALIDATED,"P8_BOUNDED_PROFILE",kP8}};
constexpr PrimitiveFamilyDescriptor kFamilies[]={{kFamily,kDomain,ValidationState::VALIDATED,"P8_SEGMENTED_RESIDENCY_GRAPH_BINDING",kP8}};
constexpr CapabilityDescriptor kCaps[]={{kCapability,kDomain,kSegmented,{},ValidationState::VALIDATED,"P8_BOUNDED_SEGMENTED_CAPABILITY",kP8}};
constexpr PrimitiveDescriptor kPrimitives[]={
 {kSegmented,kFamily,kCapability,kSegmentedRepresentation,5347770372ull,
  PRIMITIVE_REQUIRES_RESIDENCY|PRIMITIVE_REQUIRES_IDENTITY_VALIDATION,
  ValidationState::VALIDATED,"P8_SEGMENTED_RESIDENT_GRAPH_BINDING",kP8}
};
constexpr AcquisitionPathDescriptor kAcq[]={
 {kBuildResidency,kSegmented,0u,AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY,
  ValidationState::VALIDATED,"P8_SEGMENTED_ALLOCATE_COPY_RESIDENCY",kP8}
};
constexpr LifecycleDescriptor kLife[]={{kSegmented,true,true,true,true,true,false,kP8}};
constexpr RegistrationBundle kBundle{
 "P8_SEGMENTED_REFERENCE_REGISTRATION_V2",
 kDomains,1,nullptr,0,kEvidence,1,kProfiles,1,kFamilies,1,kCaps,1,
 kPrimitives,1,kAcq,1,nullptr,0,kLife,1
};
}
const registry_v2::RegistrationBundle& p8_segmented_bundle() noexcept{return kBundle;}
} // namespace arcllm::v1::p8_registration_v2
