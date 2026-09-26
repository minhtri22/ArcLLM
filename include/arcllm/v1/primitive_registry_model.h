#pragma once
#include <cstddef>
#include <cstdint>

namespace arcllm::v1::registry {

template <typename Tag>
struct OpaqueId {
    std::uint64_t value = 0;
    constexpr explicit operator bool() const noexcept { return value != 0; }
    friend constexpr bool operator==(OpaqueId a, OpaqueId b) noexcept { return a.value == b.value; }
    friend constexpr bool operator!=(OpaqueId a, OpaqueId b) noexcept { return !(a == b); }
};

struct DomainTag {};
struct CapabilityTag {};
struct PrimitiveFamilyTag {};
struct PrimitiveTag {};
struct RepresentationTag {};
struct EvidenceProfileTag {};
struct ReuseMetricTag {};
struct AcquisitionPathTag {};
struct EvidenceSetTag {};

using DomainId = OpaqueId<DomainTag>;
using CapabilityId = OpaqueId<CapabilityTag>;
using PrimitiveFamilyId = OpaqueId<PrimitiveFamilyTag>;
using PrimitiveId = OpaqueId<PrimitiveTag>;
using RepresentationId = OpaqueId<RepresentationTag>;
using EvidenceProfileId = OpaqueId<EvidenceProfileTag>;
using ReuseMetricId = OpaqueId<ReuseMetricTag>;
using AcquisitionPathId = OpaqueId<AcquisitionPathTag>;
using EvidenceSetId = OpaqueId<EvidenceSetTag>;

enum class ValidationState : std::uint8_t {
    VALIDATED = 0,
    DISABLED_BY_EVIDENCE = 1,
    CAPABILITY_GATED = 2,
    HISTORICAL_REFERENCE = 3
};

enum PrimitiveFlags : std::uint32_t {
    PRIMITIVE_NONE = 0u,
    PRIMITIVE_FALLBACK = 1u << 0,
    PRIMITIVE_NO_EXTRA_REPRESENTATION = 1u << 1,
    PRIMITIVE_REQUIRES_RESIDENCY = 1u << 2,
    PRIMITIVE_REQUIRES_IDENTITY_VALIDATION = 1u << 3
};

struct ProvenanceRef {
    const char* artifact_path = nullptr;
    const char* git_blob = nullptr;
};

struct DomainDescriptor {
    DomainId id{};
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct ReuseMetricDescriptor {
    ReuseMetricId id{};
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct EvidenceSetDescriptor {
    EvidenceSetId id{};
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct EvidenceProfileDescriptor {
    EvidenceProfileId id{};
    DomainId domain{};
    ReuseMetricId reuse_metric{};
    EvidenceSetId evidence_set{};
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct PrimitiveFamilyDescriptor {
    PrimitiveFamilyId id{};
    DomainId domain{};
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct CapabilityDescriptor {
    CapabilityId id{};
    DomainId domain{};
    PrimitiveId fallback_primitive{};
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct PrimitiveDescriptor {
    PrimitiveId id{};
    PrimitiveFamilyId family{};
    CapabilityId capability{};
    RepresentationId representation{};
    std::uint64_t residency_bytes = 0;
    std::uint32_t flags = PRIMITIVE_NONE;
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct AcquisitionPathDescriptor {
    AcquisitionPathId id{};
    PrimitiveId target_primitive{};
    std::uint32_t priority = 0;
    ValidationState state = ValidationState::VALIDATED;
    const char* opaque_name = nullptr;
    ProvenanceRef provenance{};
};

struct AcquisitionThresholdDescriptor {
    CapabilityId capability{};
    EvidenceProfileId profile{};
    AcquisitionPathId acquisition{};
    ReuseMetricId reuse_metric{};
    std::uint64_t minimum_reuse_units = 0;
    ValidationState state = ValidationState::VALIDATED;
    ProvenanceRef provenance{};
};

struct ResidentPreferenceDescriptor {
    CapabilityId capability{};
    EvidenceProfileId profile{};
    PrimitiveId preferred_when_resident{};
    PrimitiveId fallback_primitive{};
    ValidationState state = ValidationState::VALIDATED;
    ProvenanceRef provenance{};
};

struct LifecycleDescriptor {
    PrimitiveId primitive{};
    bool preserve_residency_outside_domain = false;
    bool evict_on_lease_revoke = false;
    bool evict_on_identity_invalid = false;
    bool evict_on_execution_unavailable = false;
    bool evict_on_model_unload = false;
    bool evict_on_zero_future_reuse = false;
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
    const ResidentPreferenceDescriptor* resident_preferences = nullptr;
    std::size_t resident_preference_count = 0;
    const LifecycleDescriptor* lifecycles = nullptr;
    std::size_t lifecycle_count = 0;
};

} // namespace arcllm::v1::registry
