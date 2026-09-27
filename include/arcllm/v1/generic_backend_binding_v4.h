#pragma once
#include "generic_policy_engine_v4.h"
#include <cstddef>
#include <cstdint>

namespace arcllm::v1::binding_v4 {

using registry_v2::AcquisitionPathId;
using registry_v2::CapabilityId;
using registry_v2::PrimitiveId;
using registry_v2::PrimitiveRegistry;
using policy_v4::PolicyDecision;

struct RepresentationHandle {
    std::uint64_t opaque = 0;
    constexpr explicit operator bool() const noexcept { return opaque != 0; }
};

struct PrimitiveHandle {
    std::uint64_t opaque = 0;
    constexpr explicit operator bool() const noexcept { return opaque != 0; }
};

enum class BackendStatus : std::uint8_t {
    OK = 0,
    UNAVAILABLE = 1,
    RESOURCE_DENIED = 2,
    ACQUISITION_FAILED = 3,
    VALIDATION_FAILED = 4,
    RELEASE_FAILED = 5,
    PRIMITIVE_UNAVAILABLE = 6,
    INVALID_BINDING_STATE = 7
};

class BackendAdapter {
public:
    virtual ~BackendAdapter() = default;
    virtual BackendStatus acquire(
        AcquisitionPathId path,
        PrimitiveId target,
        std::uint64_t residency_bytes,
        RepresentationHandle& out_representation) noexcept = 0;
    virtual BackendStatus validate(
        RepresentationHandle representation,
        CapabilityId capability,
        PrimitiveId target) noexcept = 0;
    virtual BackendStatus release(
        PrimitiveId target,
        RepresentationHandle representation) noexcept = 0;
    virtual BackendStatus resolve_primitive(
        PrimitiveId primitive,
        PrimitiveHandle& out_primitive) noexcept = 0;
};

struct RepresentationSlot {
    PrimitiveId primitive{};
    RepresentationHandle representation{};
};

struct BindingState {
    static constexpr std::size_t kMaxRepresentations = 64;
    RepresentationSlot slots[kMaxRepresentations]{};

    RepresentationHandle find(PrimitiveId primitive) const noexcept;
    bool set(PrimitiveId primitive, RepresentationHandle representation) noexcept;
    void clear(PrimitiveId primitive) noexcept;
};

struct ApplyResult {
    PolicyDecision decision{};
    BackendStatus backend_status = BackendStatus::OK;
    BackendStatus cleanup_status = BackendStatus::OK;
    RepresentationHandle unreleased_representation{};
    PrimitiveHandle primitive{};
    bool ready = false;
};

ApplyResult apply_decision(
    const PrimitiveRegistry& registry,
    CapabilityId capability,
    const PolicyDecision& decision,
    BackendAdapter& backend,
    BindingState& state) noexcept;

} // namespace arcllm::v1::binding_v4
