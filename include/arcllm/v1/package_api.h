#pragma once
#include <cstdint>

namespace arcllm::v1::package {

struct ApiVersion {
    std::uint16_t major;
    std::uint16_t minor;
};
inline constexpr ApiVersion kApiVersion{1u, 0u};

enum class CapabilityId : std::uint32_t {
    Q4K_DECODE_FFN_DOWN = 0x00010001u
};

// Opaque evidence profiles. PROFILE_0/1 preserve the frozen W-S/W-C evidence
// without inventing a semantic workload classifier that has not been validated.
enum class EvidenceProfileId : std::uint32_t {
    PROFILE_0 = 0x00005101u,
    PROFILE_1 = 0x00005102u
};

enum class PrimitiveId : std::uint32_t {
    A_SPLIT_K32 = 0x0000A001u,
    B_EXEC148 = 0x0000B001u
};

enum class AcquisitionId : std::uint32_t {
    NONE = 0u,
    GPU_IN_PLACE = 0x0000C101u,
    COLD_UNBUFFERED_SIDECAR = 0x0000C102u,
    CPU_DIRECT = 0x0000C103u
};

enum class LifecycleAction : std::uint8_t {
    NONE = 0,
    ACQUIRE = 1,
    EVICT = 2
};

enum class PlanStatus : std::uint8_t {
    OK = 0,
    UNSUPPORTED_CAPABILITY = 1,
    UNSUPPORTED_PROFILE = 2
};

enum class PlanReason : std::uint8_t {
    OUTSIDE_VALIDATED_DOMAIN = 0,
    RESIDENT_B_INVALID = 1,
    RESIDENCY_LEASE_REVOKED = 2,
    NO_FUTURE_REUSE = 3,
    RESIDENT_B_REUSED = 4,
    FUTURE_REUSE_UNKNOWN = 5,
    RESIDENCY_LEASE_DENIED = 6,
    ACQUISITION_VETOED = 7,
    PRIMARY_THRESHOLD_MET = 8,
    PRIMARY_BELOW_THRESHOLD = 9,
    SECONDARY_THRESHOLD_MET = 10,
    SECONDARY_BELOW_THRESHOLD = 11,
    TERTIARY_THRESHOLD_MET = 12,
    NO_JUSTIFIED_B_ACQUISITION_PATH = 13,
    MODEL_UNAVAILABLE = 14,
    UNSUPPORTED_REQUEST = 15
};

struct ReuseEstimate {
    bool known = false;
    std::uint64_t tokens = 0;
};

struct RuntimeState {
    bool current_request_in_validated_domain = true;
    bool model_loaded = true;

    bool b_resident = false;
    bool b_identity_valid = false;
    bool b_execution_available = true;
    bool residency_lease_granted = false;

    ReuseEstimate future_reuse{};

    bool acquisition_allowed = true;
    bool primary_acquisition_available = true;
    bool primary_acquisition_vetoed = false;
    bool secondary_acquisition_available = false;
    bool tertiary_acquisition_available = true;
};

struct PlanRequest {
    CapabilityId capability = CapabilityId::Q4K_DECODE_FFN_DOWN;
    EvidenceProfileId profile = EvidenceProfileId::PROFILE_0;
    RuntimeState runtime{};
};

struct Plan {
    PlanStatus status = PlanStatus::OK;
    PrimitiveId route = PrimitiveId::A_SPLIT_K32;
    LifecycleAction lifecycle = LifecycleAction::NONE;
    AcquisitionId acquisition = AcquisitionId::NONE;
    PlanReason reason = PlanReason::NO_JUSTIFIED_B_ACQUISITION_PATH;
    std::uint64_t threshold_tokens = 0;
    std::uint64_t requested_residency_bytes = 0;
    bool preserve_existing_b = false;
    bool require_post_acquisition_identity_validation = false;
};

struct ProfileDescriptor {
    EvidenceProfileId id;
    std::uint64_t primary_create_threshold_tokens;
    std::uint64_t secondary_create_threshold_tokens;
    std::uint64_t tertiary_create_threshold_tokens;
};

struct CapabilityDescriptor {
    CapabilityId id;
    ApiVersion api_version;
    PrimitiveId fallback_primitive;
    PrimitiveId represented_primitive;
    std::uint64_t represented_residency_bytes;
    ProfileDescriptor profiles[2];
    std::uint32_t profile_count;
};

const CapabilityDescriptor& capability_descriptor() noexcept;
Plan plan(const PlanRequest& request) noexcept;

} // namespace arcllm::v1::package
