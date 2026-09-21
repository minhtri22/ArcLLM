#pragma once
#include "q6cb_q6_reference.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace q6cb {

enum class ConditioningStratum {
    LOW_CANCELLATION_BOUNDED_RANGE,
    HIGH_CANCELLATION,
    SCALE_HETEROGENEITY,
    MIXED_SIGN_HIGH_DYNAMIC_RANGE,
    NEUTRAL_RANDOM_CONTROL
};

struct FixtureSpec {
    std::string id;
    uint32_t n = 0;
    uint32_t rows = 0;
    uint64_t seed = 0;
    ConditioningStratum stratum = ConditioningStratum::NEUTRAL_RANDOM_CONTROL;
};

struct Fixture {
    FixtureSpec spec;
    std::vector<uint8_t> packed;
    std::vector<float> x;
};

constexpr uint64_t kForbiddenSa1Seeds[4] = {
    0x5341315046495831ull, 0x5341315046495832ull,
    0x5341315046495833ull, 0x5341315046495834ull
};

inline uint64_t splitmix64(uint64_t& x) {
    uint64_t z = (x += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30u)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27u)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31u);
}

inline float unit_signed(uint64_t& s) {
    const int32_t v = int32_t(splitmix64(s) >> 33u) - int32_t(1u << 30u);
    return float(v) / float(1u << 30u);
}

inline void validate_fixture_spec(const FixtureSpec& s) {
    if (s.id.empty()) throw std::invalid_argument("fixture id required");
    if (s.n == 0u || (s.n % kQ6ValuesPerBlock) != 0u)
        throw std::invalid_argument("fixture n must be multiple of 256");
    if (s.rows == 0u) throw std::invalid_argument("fixture rows required");
    if ((s.n == 3584u && s.rows == 512u) ||
        (s.n == 18944u && s.rows == 3584u))
        throw std::invalid_argument("exact SA1 Q6 cell forbidden");
    for (uint64_t v : kForbiddenSa1Seeds)
        if (s.seed == v) throw std::invalid_argument("SA1 seed reuse forbidden");
}

inline void put_u16_le(uint8_t* p, uint16_t v) {
    p[0] = uint8_t(v & 255u);
    p[1] = uint8_t(v >> 8u);
}

inline void set_q6_code(uint8_t* block, uint32_t k, uint32_t code) {
    if (k >= 256u || code > 63u) throw std::out_of_range("Q6 code");
    const uint32_t half = k >> 7u;
    const uint32_t kk = k & 127u;
    const uint32_t l = kk & 31u;
    const uint32_t quarter = kk >> 5u;
    uint8_t* ql = block + half * 64u;
    uint8_t* qh = block + 128u + half * 32u;
    const uint8_t lo = uint8_t(code & 15u);
    const uint8_t hi = uint8_t((code >> 4u) & 3u);
    if (quarter == 0u) {
        ql[l] = uint8_t((ql[l] & 0xF0u) | lo);
        qh[l] = uint8_t((qh[l] & 0xFCu) | hi);
    } else if (quarter == 1u) {
        ql[l + 32u] = uint8_t((ql[l + 32u] & 0xF0u) | lo);
        qh[l] = uint8_t((qh[l] & 0xF3u) | uint8_t(hi << 2u));
    } else if (quarter == 2u) {
        ql[l] = uint8_t((ql[l] & 0x0Fu) | uint8_t(lo << 4u));
        qh[l] = uint8_t((qh[l] & 0xCFu) | uint8_t(hi << 4u));
    } else {
        ql[l + 32u] = uint8_t((ql[l + 32u] & 0x0Fu) | uint8_t(lo << 4u));
        qh[l] = uint8_t((qh[l] & 0x3Fu) | uint8_t(hi << 6u));
    }
}

inline uint32_t scale_slot_for_k(uint32_t k) {
    const uint32_t kk = k & 127u;
    const uint32_t l = kk & 31u;
    const uint32_t quarter = kk >> 5u;
    return (k >> 7u) * 8u + (l >> 4u) + quarter * 2u;
}

inline int choose_scale(ConditioningStratum st, uint32_t slot, uint64_t& s) {
    switch (st) {
        case ConditioningStratum::LOW_CANCELLATION_BOUNDED_RANGE:
            return 1 + int(slot % 4u);
        case ConditioningStratum::HIGH_CANCELLATION:
            return (slot & 1u) ? -32 : 32;
        case ConditioningStratum::SCALE_HETEROGENEITY: {
            static constexpr int vals[8] = {1, 3, 7, 15, 31, 63, 95, 127};
            int v = vals[slot & 7u];
            return (slot & 8u) ? -v : v;
        }
        case ConditioningStratum::MIXED_SIGN_HIGH_DYNAMIC_RANGE:
            return (slot & 1u) ? -127 : 127;
        default:
            return int(int8_t(uint8_t(splitmix64(s) & 255u)));
    }
}

inline int choose_q(ConditioningStratum st, uint32_t k, uint64_t& s) {
    switch (st) {
        case ConditioningStratum::LOW_CANCELLATION_BOUNDED_RANGE:
            return 1 + int(k % 7u);
        case ConditioningStratum::HIGH_CANCELLATION:
            return (k & 1u) ? -31 : 31;
        case ConditioningStratum::SCALE_HETEROGENEITY:
            return int(k % 9u) - 4;
        case ConditioningStratum::MIXED_SIGN_HIGH_DYNAMIC_RANGE:
            return (k % 4u < 2u) ? 31 : -32;
        default:
            return int(splitmix64(s) % 64u) - 32;
    }
}

inline float choose_x(ConditioningStratum st, uint32_t k, uint64_t& s) {
    switch (st) {
        case ConditioningStratum::LOW_CANCELLATION_BOUNDED_RANGE:
            return 0.25f + 0.5f * float((k % 17u) + 1u) / 18.0f;
        case ConditioningStratum::HIGH_CANCELLATION:
            return (k & 1u) ? -1.0f : 1.0f;
        case ConditioningStratum::SCALE_HETEROGENEITY:
            return (k & 1u) ? -0.5f : 0.5f;
        case ConditioningStratum::MIXED_SIGN_HIGH_DYNAMIC_RANGE: {
            static constexpr float vals[8] = {0.015625f,-0.03125f,0.125f,-0.25f,1.0f,-2.0f,8.0f,-16.0f};
            return vals[k & 7u];
        }
        default:
            return unit_signed(s);
    }
}

inline Fixture generate_fixture(const FixtureSpec& spec) {
    validate_fixture_spec(spec);
    Fixture f;
    f.spec = spec;
    const uint32_t blocks = spec.n / 256u;
    const size_t row_bytes = size_t(blocks) * kQ6BlockBytes;
    f.packed.assign(row_bytes * spec.rows, 0u);
    f.x.resize(spec.n);

    uint64_t s = spec.seed ^ 0x513643425F465831ull;
    for (uint32_t r = 0; r < spec.rows; ++r) {
        for (uint32_t ib = 0; ib < blocks; ++ib) {
            uint8_t* block = f.packed.data() + size_t(r) * row_bytes + size_t(ib) * kQ6BlockBytes;
            for (uint32_t slot = 0; slot < 16u; ++slot) {
                const int sc = choose_scale(spec.stratum, slot + ib * 17u + r * 31u, s);
                block[192u + slot] = uint8_t(int8_t(sc));
            }
            // d = 1.0 in binary16. Conditioning is expressed by q/scale/x, not by searching d.
            put_u16_le(block + 208u, 0x3C00u);
            for (uint32_t k = 0; k < 256u; ++k) {
                const uint32_t global_k = ib * 256u + k;
                const int q = choose_q(spec.stratum, global_k + r * 13u, s);
                set_q6_code(block, k, uint32_t(q + 32));
            }
        }
    }
    for (uint32_t k = 0; k < spec.n; ++k)
        f.x[k] = choose_x(spec.stratum, k, s);
    return f;
}

inline const char* stratum_name(ConditioningStratum s) {
    switch (s) {
        case ConditioningStratum::LOW_CANCELLATION_BOUNDED_RANGE: return "LOW_CANCELLATION_BOUNDED_RANGE";
        case ConditioningStratum::HIGH_CANCELLATION: return "HIGH_CANCELLATION";
        case ConditioningStratum::SCALE_HETEROGENEITY: return "SCALE_HETEROGENEITY";
        case ConditioningStratum::MIXED_SIGN_HIGH_DYNAMIC_RANGE: return "MIXED_SIGN_HIGH_DYNAMIC_RANGE";
        default: return "NEUTRAL_RANDOM_CONTROL";
    }
}

inline ConditioningStratum parse_stratum(const std::string& s) {
    if (s == "LOW_CANCELLATION_BOUNDED_RANGE") return ConditioningStratum::LOW_CANCELLATION_BOUNDED_RANGE;
    if (s == "HIGH_CANCELLATION") return ConditioningStratum::HIGH_CANCELLATION;
    if (s == "SCALE_HETEROGENEITY") return ConditioningStratum::SCALE_HETEROGENEITY;
    if (s == "MIXED_SIGN_HIGH_DYNAMIC_RANGE") return ConditioningStratum::MIXED_SIGN_HIGH_DYNAMIC_RANGE;
    if (s == "NEUTRAL_RANDOM_CONTROL") return ConditioningStratum::NEUTRAL_RANDOM_CONTROL;
    throw std::invalid_argument("unknown conditioning stratum");
}

} // namespace q6cb
