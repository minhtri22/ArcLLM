#pragma once
#include "package_api.h"

namespace arcllm::v1::package {

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
    PRIMITIVE_UNAVAILABLE = 6
};

class BackendAdapter {
public:
    virtual ~BackendAdapter() = default;

    virtual BackendStatus acquire(
        AcquisitionId path,
        std::uint64_t residency_bytes,
        RepresentationHandle& out_representation) noexcept = 0;

    virtual BackendStatus validate(
        RepresentationHandle representation,
        CapabilityId capability) noexcept = 0;

    virtual BackendStatus release(
        RepresentationHandle representation) noexcept = 0;

    virtual BackendStatus resolve_primitive(
        PrimitiveId primitive,
        PrimitiveHandle& out_primitive) noexcept = 0;
};

struct PackageState {
    RepresentationHandle b_representation{};
};

struct ApplyResult {
    Plan plan{};
    BackendStatus backend_status = BackendStatus::OK;
    PrimitiveHandle primitive{};
    bool ready = false;
    bool fell_back_to_a = false;
};

// Applies exactly one lifecycle decision. It never performs an automatic
// alternate-acquisition retry chain.
ApplyResult apply_plan(
    const Plan& plan,
    BackendAdapter& backend,
    PackageState& state) noexcept;

} // namespace arcllm::v1::package
