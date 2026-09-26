#include "arcllm_v1_q4k_down_reference_registration_v2.h"

namespace arcllm::v1::reference_registration_v2 {
namespace {
using namespace registry_v2;

constexpr ProvenanceRef kArchitecture{
 "artifacts/ARCLLM_V1/ARCLLM_V1_Q4_DOWN_PRIMITIVE_ARCHITECTURE_LOCK_v0.1.json",
 "5c696b924038de13de8d5176b724fcd814cdcc8e"
};
constexpr ProvenanceRef kPerformance{
 "artifacts/ARCLLM_V1/ARCLLM_V1_B1_2_P1_P3_PERFORMANCE_CANONICAL_v0.1.json",
 "339053906653e522cf711f43cc0a5fccad6e8f14"
};
constexpr ProvenanceRef kPolicy{
 "artifacts/ARCLLM_V1/ARCLLM_V1_B1_SELECTION_LIFETIME_POLICY_INTEGRATION_v0.1.json",
 "c90fb7386bbaae6fbca03aeefe6a9acd119e371d"
};

constexpr DomainDescriptor kDomains[]={{kDomain,ValidationState::VALIDATED,"Q4K_DECODE_FFN_DOWN_FROZEN_DOMAIN",kArchitecture}};
constexpr ReuseMetricDescriptor kMetrics[]={{kReuseMetric,ValidationState::VALIDATED,"FUTURE_REUSE_TOKENS_FROZEN_METRIC",kPolicy}};
constexpr EvidenceSetDescriptor kEvidence[]={{kEvidenceSet,ValidationState::VALIDATED,"B1_2_FROZEN_ACQUISITION_AND_STEADY_STATE_EVIDENCE",kPerformance}};
constexpr EvidenceProfileDescriptor kProfiles[]={
 {kProfile0,kDomain,kReuseMetric,kEvidenceSet,ValidationState::VALIDATED,"PROFILE_0",kPerformance},
 {kProfile1,kDomain,kReuseMetric,kEvidenceSet,ValidationState::VALIDATED,"PROFILE_1",kPerformance}
};
constexpr PrimitiveFamilyDescriptor kFamilies[]={{kFamily,kDomain,ValidationState::VALIDATED,"Q4K_DOWN_REFERENCE_FAMILY",kArchitecture}};
constexpr CapabilityDescriptor kCaps[]={{kCapability,kDomain,kPrimitiveB,kPrimitiveA,ValidationState::VALIDATED,"Q4K_DECODE_FFN_DOWN",kArchitecture}};
constexpr PrimitiveDescriptor kPrimitives[]={
 {kPrimitiveA,kFamily,kCapability,{},0,PRIMITIVE_FALLBACK|PRIMITIVE_NO_EXTRA_REPRESENTATION,ValidationState::VALIDATED,"A_SPLIT_K32",kArchitecture},
 {kPrimitiveB,kFamily,kCapability,kRepresentationB,549527552ull,PRIMITIVE_REQUIRES_RESIDENCY|PRIMITIVE_REQUIRES_IDENTITY_VALIDATION,ValidationState::VALIDATED,"B_EXEC148",kPerformance}
};
constexpr AcquisitionPathDescriptor kAcq[]={
 {kAcquirePrimary,kPrimitiveB,0u,AcquisitionTrigger::REUSE_AMORTIZED,ValidationState::VALIDATED,"P1_GPU_IN_PLACE",kPerformance},
 {kAcquireSecondary,kPrimitiveB,1u,AcquisitionTrigger::REUSE_AMORTIZED,ValidationState::VALIDATED,"P3_COLD_UNBUFFERED",kPerformance},
 {kAcquireTertiary,kPrimitiveB,2u,AcquisitionTrigger::REUSE_AMORTIZED,ValidationState::VALIDATED,"P0_CPU_DIRECT",kPerformance},
 {kAcquireDisabledWarm,kPrimitiveB,3u,AcquisitionTrigger::REUSE_AMORTIZED,ValidationState::DISABLED_BY_EVIDENCE,"P3_WARM_PAGE_CACHE",kPerformance}
};
constexpr AcquisitionThresholdDescriptor kThresholds[]={
 {kCapability,kProfile0,kAcquirePrimary,kReuseMetric,2ull,ValidationState::VALIDATED,kPolicy},
 {kCapability,kProfile1,kAcquirePrimary,kReuseMetric,4ull,ValidationState::VALIDATED,kPolicy},
 {kCapability,kProfile0,kAcquireSecondary,kReuseMetric,15ull,ValidationState::VALIDATED,kPolicy},
 {kCapability,kProfile1,kAcquireSecondary,kReuseMetric,33ull,ValidationState::VALIDATED,kPolicy},
 {kCapability,kProfile0,kAcquireTertiary,kReuseMetric,17ull,ValidationState::VALIDATED,kPolicy},
 {kCapability,kProfile1,kAcquireTertiary,kReuseMetric,37ull,ValidationState::VALIDATED,kPolicy}
};
constexpr LifecycleDescriptor kLife[]={{kPrimitiveB,true,true,true,true,true,true,kPolicy}};
constexpr RegistrationBundle kBundle{
 "Q4K_DOWN_REFERENCE_REGISTRATION_V2",
 kDomains,1,kMetrics,1,kEvidence,1,kProfiles,2,kFamilies,1,kCaps,1,
 kPrimitives,2,kAcq,4,kThresholds,6,kLife,1
};
}
const registry_v2::RegistrationBundle& q4k_down_reference_bundle() noexcept{return kBundle;}
} // namespace arcllm::v1::reference_registration_v2
