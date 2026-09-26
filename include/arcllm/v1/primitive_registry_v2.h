#pragma once
#include "primitive_registry_model.h"
#include <cstddef>
#include <cstdint>

namespace arcllm::v1::registry_v2 {

using registry::OpaqueId;
using registry::DomainId;
using registry::CapabilityId;
using registry::PrimitiveFamilyId;
using registry::PrimitiveId;
using registry::RepresentationId;
using registry::EvidenceProfileId;
using registry::ReuseMetricId;
using registry::AcquisitionPathId;
using registry::EvidenceSetId;
using registry::ValidationState;
using registry::PrimitiveFlags;
using registry::PRIMITIVE_NONE;
using registry::PRIMITIVE_FALLBACK;
using registry::PRIMITIVE_NO_EXTRA_REPRESENTATION;
using registry::PRIMITIVE_REQUIRES_RESIDENCY;
using registry::PRIMITIVE_REQUIRES_IDENTITY_VALIDATION;
using registry::ProvenanceRef;
using registry::DomainDescriptor;
using registry::ReuseMetricDescriptor;
using registry::EvidenceSetDescriptor;
using registry::PrimitiveFamilyDescriptor;
using registry::PrimitiveDescriptor;
using registry::AcquisitionThresholdDescriptor;
using registry::LifecycleDescriptor;

enum class AcquisitionTrigger : std::uint8_t {
    REUSE_AMORTIZED = 0,
    MANDATORY_FOR_FEASIBILITY = 1
};

struct EvidenceProfileDescriptor {
    EvidenceProfileId id{};
    DomainId domain{};
    ReuseMetricId reuse_metric{}; // optional; required only by reuse-amortized acquisition
    EvidenceSetId evidence_set{};
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct CapabilityDescriptor {
    CapabilityId id{};
    DomainId domain{};
    PrimitiveId preferred_primitive{};
    PrimitiveId fallback_primitive{}; // optional; zero means no validated fallback
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct AcquisitionPathDescriptor {
    AcquisitionPathId id{};
    PrimitiveId target_primitive{};
    std::uint32_t priority = 0;
    AcquisitionTrigger trigger = AcquisitionTrigger::REUSE_AMORTIZED;
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct RegistrationBundle {
    const char* bundle_name = nullptr;
    const DomainDescriptor* domains = nullptr;
    std::size_t domain_count = 0;
    const ReuseMetricDescriptor* reuse_metrics = nullptr;
    std::size_t reuse_metric_count = 0;
    const EvidenceSetDescriptor* evidence_sets = nullptr;
    std::size_t evidence_set_count = 0;
    const EvidenceProfileDescriptor* profiles = nullptr;
    std::size_t profile_count = 0;
    const PrimitiveFamilyDescriptor* families = nullptr;
    std::size_t family_count = 0;
    const CapabilityDescriptor* capabilities = nullptr;
    std::size_t capability_count = 0;
    const PrimitiveDescriptor* primitives = nullptr;
    std::size_t primitive_count = 0;
    const AcquisitionPathDescriptor* acquisitions = nullptr;
    std::size_t acquisition_count = 0;
    const AcquisitionThresholdDescriptor* thresholds = nullptr;
    std::size_t threshold_count = 0;
    const LifecycleDescriptor* lifecycles = nullptr;
    std::size_t lifecycle_count = 0;
};

enum class RegistryStatus : std::uint8_t {
    OK = 0,
    CAPACITY_EXCEEDED = 1,
    INVALID_BUNDLE = 2,
    DUPLICATE_ID = 3,
    INVALID_REFERENCE = 4,
    DUPLICATE_POLICY_KEY = 5,
    INVALID_ACQUISITION_SEMANTICS = 6
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
    const LifecycleDescriptor* find_lifecycle(PrimitiveId primitive) const noexcept;

private:
    const RegistrationBundle* bundles_[kMaxBundles]{};
    std::size_t bundle_count_ = 0;
};

} // namespace arcllm::v1::registry_v2
