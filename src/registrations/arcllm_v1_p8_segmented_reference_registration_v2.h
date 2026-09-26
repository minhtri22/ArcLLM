#pragma once
#include "../../include/arcllm/v1/primitive_registry_v2.h"

namespace arcllm::v1::p8_registration_v2 {

inline constexpr registry_v2::DomainId kDomain{0xD8001001ull};
inline constexpr registry_v2::EvidenceSetId kEvidenceSet{0xE8001001ull};
inline constexpr registry_v2::EvidenceProfileId kProfile{0x58001001ull};
inline constexpr registry_v2::PrimitiveFamilyId kFamily{0xF8001001ull};
inline constexpr registry_v2::CapabilityId kCapability{0x18001001ull};
inline constexpr registry_v2::PrimitiveId kSegmented{0xB8001001ull};
inline constexpr registry_v2::RepresentationId kSegmentedRepresentation{0xE8001148ull};
inline constexpr registry_v2::AcquisitionPathId kBuildResidency{0xC8001001ull};

const registry_v2::RegistrationBundle& p8_segmented_bundle() noexcept;

} // namespace arcllm::v1::p8_registration_v2
