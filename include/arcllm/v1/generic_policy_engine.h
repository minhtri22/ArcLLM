#pragma once
#include "primitive_registry_api.h"

namespace arcllm::v1::policy {

using registry::AcquisitionPathId;
using registry::CapabilityId;
using registry::EvidenceProfileId;
using registry::PrimitiveId;
using registry::PrimitiveRegistry;

enum class PolicyStatus : std::uint8_t {
    OK = 0,
    UNSUPPORTED_CAPABILITY = 1,
    UNSUPPORTED_PROFILE = 2,
    PROFILE_DOMAIN_MISMATCH = 3,
    REGISTRY_POLICY_INCOMPLETE = 4,
    INVALID_RUNTIME_STATE = 5
};

enum class LifecycleAction : std::uint8_t {
    NONE = 0,
    ACQUIRE = 1,
    EVICT = 2
};

enum class PolicyReason : std::uint8_t {
    FALLBACK_ONLY = 0,
    OUTSIDE_VALIDATED_DOMAIN = 1,
    MODEL_UNAVAILABLE = 2,
    RESIDENT_PRIMITIVE_INVALID = 3,
    RESIDENT_EXECUTION_UNAVAILABLE = 4,
    RESIDENCY_LEASE_REVOKED = 5,
    NO_FUTURE_REUSE = 6,
    RESIDENT_PRIMITIVE_REUSED = 7,
    FUTURE_REUSE_UNKNOWN = 8,
    ACQUISITION_GLOBALLY_VETOED = 9,
    NO_AVAILABLE_ACQUISITION_PATH = 10,
    BELOW_ALL_ACTIVE_THRESHOLDS = 11,
    ACQUISITION_THRESHOLD_MET = 12,
    UNSUPPORTED_REQUEST = 13
};

struct PrimitiveRuntimeState {
    PrimitiveId primitive{};
    bool resident = false;
    bool identity_valid = false;
    bool execution_available = false;
    bool residency_lease_granted = false;
};

struct AcquisitionRuntimeState {
    AcquisitionPathId acquisition{};
    bool available = false;
    bool vetoed = false;
};

struct PolicyRequest {
    CapabilityId capability{};
    EvidenceProfileId profile{};

    bool current_request_in_validated_domain = true;
    bool model_loaded = true;

    bool future_reuse_known = false;
    std::uint64_t future_reuse_units = 0;

    bool acquisition_allowed = true;

    const PrimitiveRuntimeState* primitive_states = nullptr;
    std::size_t primitive_state_count = 0;

    const AcquisitionRuntimeState* acquisition_states = nullptr;
    std::size_t acquisition_state_count = 0;
};

struct PolicyDecision {
    PolicyStatus status = PolicyStatus::OK;
    PrimitiveId route{};
    LifecycleAction lifecycle = LifecycleAction::NONE;
    AcquisitionPathId acquisition{};
    PolicyReason reason = PolicyReason::FALLBACK_ONLY;

    std::uint64_t threshold_units = 0;
    std::uint64_t requested_residency_bytes = 0;

    bool preserve_existing_representation = false;
    bool require_post_acquisition_identity_validation = false;
};

PolicyDecision evaluate(
    const PrimitiveRegistry& registry,
    const PolicyRequest& request) noexcept;

} // namespace arcllm::v1::policy
