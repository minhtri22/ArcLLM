#pragma once
#include "../../include/arcllm/v1/primitive_registry_api.h"

namespace arcllm::v1::reference_registration {

inline constexpr registry::DomainId kDomain{0xD0010001ull};
inline constexpr registry::ReuseMetricId kReuseMetric{0xD0020001ull};
inline constexpr registry::EvidenceSetId kEvidenceSet{0xE0010001ull};
inline constexpr registry::EvidenceProfileId kProfile0{0x00005101ull};
inline constexpr registry::EvidenceProfileId kProfile1{0x00005102ull};
inline constexpr registry::PrimitiveFamilyId kFamily{0xF0010001ull};
inline constexpr registry::CapabilityId kCapability{0x00010001ull};
inline constexpr registry::PrimitiveId kPrimitiveA{0x0000A001ull};
inline constexpr registry::PrimitiveId kPrimitiveB{0x0000B001ull};
inline constexpr registry::RepresentationId kRepresentationB{0xE1480001ull};
inline constexpr registry::AcquisitionPathId kAcquirePrimary{0x0000C101ull};
inline constexpr registry::AcquisitionPathId kAcquireSecondary{0x0000C102ull};
inline constexpr registry::AcquisitionPathId kAcquireTertiary{0x0000C103ull};
inline constexpr registry::AcquisitionPathId kAcquireDisabledWarm{0x0000C104ull};

const registry::RegistrationBundle& q4k_down_reference_bundle() noexcept;

} // namespace arcllm::v1::reference_registration
