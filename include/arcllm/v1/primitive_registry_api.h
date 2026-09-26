#pragma once
#include "primitive_registry_model.h"

namespace arcllm::v1::registry {

enum class RegistryStatus : std::uint8_t {
    OK = 0,
    CAPACITY_EXCEEDED = 1,
    INVALID_BUNDLE = 2,
    DUPLICATE_ID = 3,
    INVALID_REFERENCE = 4,
    DUPLICATE_POLICY_KEY = 5
};

struct RegistryError {
    RegistryStatus status = RegistryStatus::OK;
    const char* category = nullptr;
    std::uint64_t id = 0;
};

class PrimitiveRegistry {
public:
    static constexpr std::size_t kMaxBundles = 64;

    RegistryStatus add_bundle(const RegistrationBundle& bundle, RegistryError* error = nullptr) noexcept;

    std::size_t bundle_count() const noexcept { return bundle_count_; }
    const RegistrationBundle* bundle_at(std::size_t index) const noexcept;

    const DomainDescriptor* find_domain(DomainId id) const noexcept;
    const ReuseMetricDescriptor* find_reuse_metric(ReuseMetricId id) const noexcept;
    const EvidenceSetDescriptor* find_evidence_set(EvidenceSetId id) const noexcept;
    const EvidenceProfileDescriptor* find_profile(EvidenceProfileId id) const noexcept;
    const PrimitiveFamilyDescriptor* find_family(PrimitiveFamilyId id) const noexcept;
    const CapabilityDescriptor* find_capability(CapabilityId id) const noexcept;
    const PrimitiveDescriptor* find_primitive(PrimitiveId id) const noexcept;
    const AcquisitionPathDescriptor* find_acquisition(AcquisitionPathId id) const noexcept;

    const AcquisitionThresholdDescriptor* find_threshold(
        CapabilityId capability,
        EvidenceProfileId profile,
        AcquisitionPathId acquisition) const noexcept;

    const ResidentPreferenceDescriptor* find_resident_preference(
        CapabilityId capability,
        EvidenceProfileId profile) const noexcept;

    const LifecycleDescriptor* find_lifecycle(PrimitiveId primitive) const noexcept;

private:
    const RegistrationBundle* bundles_[kMaxBundles]{};
    std::size_t bundle_count_ = 0;
};

} // namespace arcllm::v1::registry
