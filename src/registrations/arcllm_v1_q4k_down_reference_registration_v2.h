#pragma once
#include "../../include/arcllm/v1/primitive_registry_v2.h"

namespace arcllm::v1::reference_registration_v2 {

inline constexpr registry_v2::DomainId kDomain{0xD0010001ull};
inline constexpr registry_v2::ReuseMetricId kReuseMetric{0xD0020001ull};
inline constexpr registry_v2::EvidenceSetId kEvidenceSet{0xE0010001ull};
inline constexpr registry_v2::EvidenceProfileId kProfile0{0x00005101ull};
inline constexpr registry_v2::EvidenceProfileId kProfile1{0x00005102ull};
inline constexpr registry_v2::PrimitiveFamilyId kFamily{0xF0010001ull};
inline constexpr registry_v2::CapabilityId kCapability{0x00010001ull};
inline constexpr registry_v2::PrimitiveId kPrimitiveA{0x0000A001ull};
inline constexpr registry_v2::PrimitiveId kPrimitiveB{0x0000B001ull};
inline constexpr registry_v2::RepresentationId kRepresentationB{0xE1480001ull};
inline constexpr registry_v2::AcquisitionPathId kAcquirePrimary{0x0000C101ull};
inline constexpr registry_v2::AcquisitionPathId kAcquireSecondary{0x0000C102ull};
inline constexpr registry_v2::AcquisitionPathId kAcquireTertiary{0x0000C103ull};
inline constexpr registry_v2::AcquisitionPathId kAcquireDisabledWarm{0x0000C104ull};

const registry_v2::RegistrationBundle& q4k_down_reference_bundle() noexcept;

} // namespace arcllm::v1::reference_registration_v2
