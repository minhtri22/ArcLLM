#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace q6cb {

constexpr uint32_t kQ6ValuesPerBlock = 256;
constexpr uint32_t kQ6BlockBytes = 210;
constexpr uint32_t kSplitLanes = 32;

inline uint16_t load_u16_le(const uint8_t* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8u);
}

inline float half_to_float(uint16_t h) {
    const uint32_t sign = (h >> 15u) & 1u;
    const uint32_t exp = (h >> 10u) & 31u;
    const uint32_t frac = h & 1023u;
    if (exp == 0u) {
        if (frac == 0u) return sign ? -0.0f : 0.0f;
        const float v = std::ldexp(float(frac), -24);
        return sign ? -v : v;
    }
    if (exp == 31u) {
        if (frac != 0u) return std::numeric_limits<float>::quiet_NaN();
        return sign ? -std::numeric_limits<float>::infinity()
                    :  std::numeric_limits<float>::infinity();
    }
    const float v = std::ldexp(1.0f + float(frac) / 1024.0f, int(exp) - 15);
    return sign ? -v : v;
}

inline int signed_i8(uint8_t v) {
    return v >= 128u ? int(v) - 256 : int(v);
}

struct CanonicalQ6Element {
    int q = 0;
    int scale = 0;
    float d = 0.0f;
    float value = 0.0f;
};

inline CanonicalQ6Element decode_q6_element(const uint8_t* block, uint32_t k) {
    if (k >= kQ6ValuesPerBlock) throw std::out_of_range("Q6 element index");
    const uint32_t half = k >> 7u;
    const uint32_t kk = k & 127u;
    const uint32_t l = kk & 31u;
    const uint32_t quarter = kk >> 5u;

    const uint8_t* ql = block + half * 64u;
    const uint8_t* qh = block + 128u + half * 32u;
    const uint8_t* sc = block + 192u + half * 8u;

    uint32_t low = 0u, high = 0u, scale_index = 0u;
    const uint32_t is = l >> 4u;
    switch (quarter) {
        case 0u:
            low = ql[l] & 15u;
            high = (qh[l] >> 0u) & 3u;
            scale_index = is + 0u;
            break;
        case 1u:
            low = ql[l + 32u] & 15u;
            high = (qh[l] >> 2u) & 3u;
            scale_index = is + 2u;
            break;
        case 2u:
            low = ql[l] >> 4u;
            high = (qh[l] >> 4u) & 3u;
            scale_index = is + 4u;
            break;
        default:
            low = ql[l + 32u] >> 4u;
            high = (qh[l] >> 6u) & 3u;
            scale_index = is + 6u;
            break;
    }

    CanonicalQ6Element e;
    e.q = int(low | (high << 4u)) - 32;
    e.scale = signed_i8(sc[scale_index]);
    e.d = half_to_float(load_u16_le(block + 208u));
    e.value = e.d * float(e.scale) * float(e.q);
    return e;
}

inline float decode_q6_weight(const std::vector<uint8_t>& packed,
                              uint32_t n,
                              uint32_t row,
                              uint32_t k) {
    if (n == 0u || (n % kQ6ValuesPerBlock) != 0u)
        throw std::invalid_argument("n must be a nonzero multiple of 256");
    if (k >= n) throw std::out_of_range("k");
    const size_t row_bytes = size_t(n / kQ6ValuesPerBlock) * kQ6BlockBytes;
    const size_t base = size_t(row) * row_bytes
                      + size_t(k / kQ6ValuesPerBlock) * kQ6BlockBytes;
    if (base + kQ6BlockBytes > packed.size())
        throw std::out_of_range("packed row/block");
    return decode_q6_element(packed.data() + base, k & 255u).value;
}

inline std::vector<float> expand_q6_rows(const std::vector<uint8_t>& packed,
                                         uint32_t n,
                                         uint32_t rows) {
    std::vector<float> out(size_t(n) * rows);
    for (uint32_t r = 0; r < rows; ++r)
        for (uint32_t k = 0; k < n; ++k)
            out[size_t(r) * n + k] = decode_q6_weight(packed, n, r, k);
    return out;
}

inline std::vector<double> arm_r64(const std::vector<float>& expanded,
                                   const std::vector<float>& x,
                                   uint32_t n,
                                   uint32_t rows) {
    if (x.size() != n || expanded.size() != size_t(n) * rows)
        throw std::invalid_argument("R64 shape");
    std::vector<double> y(rows, 0.0);
    for (uint32_t r = 0; r < rows; ++r) {
        double sum = 0.0;
        for (uint32_t k = 0; k < n; ++k)
            sum += double(expanded[size_t(r) * n + k]) * double(x[k]);
        y[r] = sum;
    }
    return y;
}

inline std::vector<float> arm_s32(const std::vector<float>& expanded,
                                  const std::vector<float>& x,
                                  uint32_t n,
                                  uint32_t rows) {
    if (x.size() != n || expanded.size() != size_t(n) * rows)
        throw std::invalid_argument("S32 shape");
    std::vector<float> y(rows, 0.0f);
    for (uint32_t r = 0; r < rows; ++r) {
        float sum = 0.0f;
        for (uint32_t k = 0; k < n; ++k)
            sum += expanded[size_t(r) * n + k] * x[k];
        y[r] = sum;
    }
    return y;
}

// Canonical deterministic proxy for the 32-way reduction topology.
// Lane partition exactly follows k = lane, lane+32, ... . Reduction is
// frozen as five pairwise binary-tree stages: +16,+8,+4,+2,+1.
inline float fixed_tree_reduce32(std::array<float, kSplitLanes> v) {
    for (uint32_t stride = 16u; stride > 0u; stride >>= 1u)
        for (uint32_t i = 0; i < stride; ++i)
            v[i] = v[i] + v[i + stride];
    return v[0];
}

inline std::vector<float> arm_t32_cpu(const std::vector<float>& expanded,
                                      const std::vector<float>& x,
                                      uint32_t n,
                                      uint32_t rows) {
    if (x.size() != n || expanded.size() != size_t(n) * rows)
        throw std::invalid_argument("T32 CPU shape");
    std::vector<float> y(rows, 0.0f);
    for (uint32_t r = 0; r < rows; ++r) {
        std::array<float, kSplitLanes> lane{};
        for (uint32_t l = 0; l < kSplitLanes; ++l)
            for (uint32_t k = l; k < n; k += kSplitLanes)
                lane[l] += expanded[size_t(r) * n + k] * x[k];
        y[r] = fixed_tree_reduce32(lane);
    }
    return y;
}

} // namespace q6cb
