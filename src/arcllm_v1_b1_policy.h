#pragma once
#include <cstdint>

namespace arcllm::v1::b1 {

inline constexpr std::uint64_t kExec148ResidentBytes = 549527552ull;

enum class Workload : std::uint8_t { WS = 0, WC = 1 };
enum class Route : std::uint8_t { A_SPLIT_K32 = 0, B_EXEC148 = 1 };
enum class Lifecycle : std::uint8_t {
    NONE = 0,
    ACQUIRE_B_P1_GPU = 1,
    ACQUIRE_B_P3_COLD = 2,
    ACQUIRE_B_P0_CPU = 3,
    EVICT_B = 4
};
enum class Reason : std::uint8_t {
    OUTSIDE_VALIDATED_DOMAIN = 0,
    RESIDENT_B_INVALID = 1,
    RESIDENCY_LEASE_REVOKED = 2,
    NO_FUTURE_REUSE = 3,
    RESIDENT_B_REUSED = 4,
    FUTURE_REUSE_UNKNOWN = 5,
    RESIDENCY_LEASE_DENIED = 6,
    ACQUISITION_VETOED = 7,
    P1_THRESHOLD_MET = 8,
    P1_BELOW_THRESHOLD = 9,
    P3_COLD_THRESHOLD_MET = 10,
    P3_COLD_BELOW_THRESHOLD = 11,
    P0_THRESHOLD_MET = 12,
    NO_JUSTIFIED_B_ACQUISITION_PATH = 13,
    MODEL_UNAVAILABLE = 14
};

struct Snapshot {
    Workload workload = Workload::WS;
    bool current_request_in_validated_domain = true;
    bool model_loaded = true;

    bool b_resident = false;
    bool b_identity_valid = false;
    bool b_execution_available = true;
    bool residency_lease_granted = false;

    bool future_reuse_known = false;
    std::uint64_t future_reuse_tokens = 0;

    bool acquisition_allowed = true;
    bool p1_available = true;
    bool p1_vetoed = false;
    bool p3_cold_available = false;
    bool p0_cpu_available = true;
};

struct Decision {
    Route route = Route::A_SPLIT_K32;
    Lifecycle lifecycle = Lifecycle::NONE;
    Reason reason = Reason::NO_JUSTIFIED_B_ACQUISITION_PATH;
    std::uint64_t threshold_tokens = 0;
    std::uint64_t residency_bytes = kExec148ResidentBytes;
    bool b_resident_after_success = false;
    bool preserve_existing_b = false;
};

constexpr std::uint64_t p1_threshold(Workload w) {
    return w == Workload::WS ? 2ull : 4ull;
}
constexpr std::uint64_t p3_cold_threshold(Workload w) {
    return w == Workload::WS ? 15ull : 33ull;
}
constexpr std::uint64_t p0_threshold(Workload w) {
    return w == Workload::WS ? 17ull : 37ull;
}

constexpr Decision use_a(Reason reason, bool preserve_existing_b = false, std::uint64_t threshold = 0) {
    return {Route::A_SPLIT_K32, Lifecycle::NONE, reason, threshold,
            kExec148ResidentBytes, preserve_existing_b, preserve_existing_b};
}
constexpr Decision use_b_resident() {
    return {Route::B_EXEC148, Lifecycle::NONE, Reason::RESIDENT_B_REUSED, 0,
            kExec148ResidentBytes, true, true};
}
constexpr Decision evict_to_a(Reason reason) {
    return {Route::A_SPLIT_K32, Lifecycle::EVICT_B, reason, 0,
            kExec148ResidentBytes, false, false};
}
constexpr Decision acquire_b(Lifecycle path, Reason reason, std::uint64_t threshold) {
    return {Route::B_EXEC148, path, reason, threshold,
            kExec148ResidentBytes, true, false};
}

constexpr Decision evaluate(const Snapshot& s) {
    // Residency validity and resource revocation are lifecycle facts, independent of
    // whether the current request is inside the measured B routing domain.
    if (s.b_resident) {
        if (!s.model_loaded || !s.b_identity_valid || !s.b_execution_available) {
            return evict_to_a(Reason::RESIDENT_B_INVALID);
        }
        if (!s.residency_lease_granted) {
            return evict_to_a(Reason::RESIDENCY_LEASE_REVOKED);
        }
        if (s.future_reuse_known && s.future_reuse_tokens == 0) {
            return evict_to_a(Reason::NO_FUTURE_REUSE);
        }

        // Route state and residency state are intentionally distinct. A request that
        // is outside the frozen B evidence uses A but does not destroy a still-valid B.
        if (!s.current_request_in_validated_domain) {
            return use_a(Reason::OUTSIDE_VALIDATED_DOMAIN, true);
        }

        // Acquisition is sunk. Do not reapply the 2/4-token creation threshold here.
        return use_b_resident();
    }

    if (!s.model_loaded) {
        return use_a(Reason::MODEL_UNAVAILABLE);
    }
    if (!s.current_request_in_validated_domain) {
        return use_a(Reason::OUTSIDE_VALIDATED_DOMAIN);
    }
    if (!s.future_reuse_known) {
        return use_a(Reason::FUTURE_REUSE_UNKNOWN);
    }
    if (s.future_reuse_tokens == 0) {
        return use_a(Reason::NO_FUTURE_REUSE);
    }
    if (!s.residency_lease_granted) {
        return use_a(Reason::RESIDENCY_LEASE_DENIED);
    }
    if (!s.acquisition_allowed) {
        return use_a(Reason::ACQUISITION_VETOED);
    }

    const std::uint64_t h = s.future_reuse_tokens;

    if (s.p1_available && !s.p1_vetoed) {
        const auto t = p1_threshold(s.workload);
        if (h >= t) return acquire_b(Lifecycle::ACQUIRE_B_P1_GPU, Reason::P1_THRESHOLD_MET, t);
        return use_a(Reason::P1_BELOW_THRESHOLD, false, t);
    }

    if (s.p3_cold_available) {
        const auto t = p3_cold_threshold(s.workload);
        if (h >= t) return acquire_b(Lifecycle::ACQUIRE_B_P3_COLD, Reason::P3_COLD_THRESHOLD_MET, t);
        return use_a(Reason::P3_COLD_BELOW_THRESHOLD, false, t);
    }

    if (s.p0_cpu_available) {
        const auto t = p0_threshold(s.workload);
        if (h >= t) return acquire_b(Lifecycle::ACQUIRE_B_P0_CPU, Reason::P0_THRESHOLD_MET, t);
    }

    return use_a(Reason::NO_JUSTIFIED_B_ACQUISITION_PATH);
}

constexpr bool same_decision(const Decision& a, const Decision& b) {
    return a.route == b.route &&
           a.lifecycle == b.lifecycle &&
           a.reason == b.reason &&
           a.threshold_tokens == b.threshold_tokens &&
           a.residency_bytes == b.residency_bytes &&
           a.b_resident_after_success == b.b_resident_after_success &&
           a.preserve_existing_b == b.preserve_existing_b;
}

} // namespace arcllm::v1::b1
