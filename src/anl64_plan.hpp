#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <stdexcept>
#include <cstddef>

enum class Anl64Role : uint16_t {
    TokenEmbedding = 0,
    AttnRmsNorm,
    QProj,
    KProj,
    VProj,
    QRope,
    KRope,
    KVStore,
    CachedGqa,
    OProj,
    AttnResidual,
    FfnRmsNorm,
    FfnGate,
    FfnUp,
    SwiGlu,
    FfnDown,
    FfnResidual,
    OutputNorm,
    LmHead
};

enum class Anl64Executor : uint16_t {
    Safe = 0,
    Q4FastFixed = 1,
    QuantSafe = 2
};

struct Anl64Region64 {
    uint32_t plan_node_index = 0;
    uint32_t row_base = 0;
    uint32_t row_count = 0;
    uint32_t reserved = 0;
    uint64_t active_mask = 0xffffffffffffffffull;
};

struct Anl64PlanNode {
    uint32_t index = 0;
    uint32_t layer = 0xffffffffu;
    uint32_t n = 0;
    uint32_t rows = 0;
    uint32_t region_begin = 0;
    uint32_t region_count = 0;
    uint16_t role = 0;
    uint16_t executor = 0;
    uint16_t quant_type = 0xffffu;
    uint16_t flags = 0;
};

static_assert(sizeof(Anl64Region64) <= 64, "Region64 descriptor exceeds P2 budget");
static_assert(sizeof(Anl64PlanNode) <= 128, "PlanNode descriptor exceeds P2 budget");

struct Anl64Plan {
    std::vector<Anl64PlanNode> nodes;
    std::vector<Anl64Region64> regions;
    uint64_t hash_fnv1a64 = 0;
    uint32_t quant_linear_nodes = 0;
    uint32_t fixed_q4_fast_nodes = 0;
    uint32_t fixed_q4_regions = 0;
    uint64_t metadata_bytes = 0;
};

inline uint64_t anl64_hash_bytes(uint64_t h, const void* p, size_t n) {
    const uint8_t* b = static_cast<const uint8_t*>(p);
    for (size_t i = 0; i < n; ++i) {
        h ^= uint64_t(b[i]);
        h *= 1099511628211ull;
    }
    return h;
}

inline void anl64_hash_plan(Anl64Plan& plan) {
    uint64_t h = 1469598103934665603ull;
    for (const auto& n : plan.nodes) h = anl64_hash_bytes(h, &n, sizeof(n));
    for (const auto& r : plan.regions) h = anl64_hash_bytes(h, &r, sizeof(r));
    plan.hash_fnv1a64 = h;
}

inline void anl64_add_node(
    Anl64Plan& plan,
    uint32_t layer,
    Anl64Role role,
    Anl64Executor executor,
    uint32_t n,
    uint32_t rows,
    uint16_t quant_type
) {
    Anl64PlanNode node;
    node.index = uint32_t(plan.nodes.size());
    node.layer = layer;
    node.n = n;
    node.rows = rows;
    node.role = uint16_t(role);
    node.executor = uint16_t(executor);
    node.quant_type = quant_type;

    const bool quant = quant_type == 12u || quant_type == 14u;
    if (quant) {
        if (rows == 0u || (rows % 64u) != 0u)
            throw std::runtime_error("ANL64 Region64 requires rows divisible by 64");
        node.region_begin = uint32_t(plan.regions.size());
        node.region_count = rows / 64u;
        ++plan.quant_linear_nodes;
        for (uint32_t r = 0; r < node.region_count; ++r) {
            plan.regions.push_back(Anl64Region64{
                node.index,
                r * 64u,
                64u,
                0u,
                0xffffffffffffffffull
            });
        }
        if (executor == Anl64Executor::Q4FastFixed) {
            ++plan.fixed_q4_fast_nodes;
            plan.fixed_q4_regions += node.region_count;
        }
    }
    plan.nodes.push_back(node);
}

inline Anl64Plan anl64_build_plan(
    const std::array<uint32_t, 28>& v_types,
    const std::array<uint32_t, 28>& down_types
) {
    constexpr uint32_t H = 3584u;
    constexpr uint32_t KV = 512u;
    constexpr uint32_t FFN = 18944u;
    constexpr uint32_t VOC = 152064u;
    constexpr uint32_t LM_CHUNK = 8192u;
    constexpr uint16_t Q4 = 12u;
    constexpr uint16_t Q6 = 14u;
    constexpr uint16_t NONE = 0xffffu;

    Anl64Plan plan;
    plan.nodes.reserve(469u);
    plan.regions.reserve(24104u);

    anl64_add_node(plan, 0xffffffffu, Anl64Role::TokenEmbedding, Anl64Executor::Safe, 0u, 0u, NONE);

    for (uint32_t l = 0; l < 28u; ++l) {
        if ((v_types[l] != Q4 && v_types[l] != Q6) ||
            (down_types[l] != Q4 && down_types[l] != Q6))
            throw std::runtime_error("ANL64 mixed-quant type outside frozen Q4/Q6 domain");

        anl64_add_node(plan, l, Anl64Role::AttnRmsNorm, Anl64Executor::Safe, 0u, 0u, NONE);
        anl64_add_node(plan, l, Anl64Role::QProj, Anl64Executor::Q4FastFixed, H, H, Q4);
        anl64_add_node(plan, l, Anl64Role::KProj, Anl64Executor::Q4FastFixed, H, KV, Q4);
        anl64_add_node(plan, l, Anl64Role::VProj, Anl64Executor::QuantSafe, H, KV, uint16_t(v_types[l]));
        anl64_add_node(plan, l, Anl64Role::QRope, Anl64Executor::Safe, 0u, 0u, NONE);
        anl64_add_node(plan, l, Anl64Role::KRope, Anl64Executor::Safe, 0u, 0u, NONE);
        anl64_add_node(plan, l, Anl64Role::KVStore, Anl64Executor::Safe, 0u, 0u, NONE);
        anl64_add_node(plan, l, Anl64Role::CachedGqa, Anl64Executor::Safe, 0u, 0u, NONE);
        anl64_add_node(plan, l, Anl64Role::OProj, Anl64Executor::Q4FastFixed, H, H, Q4);
        anl64_add_node(plan, l, Anl64Role::AttnResidual, Anl64Executor::Safe, 0u, 0u, NONE);
        anl64_add_node(plan, l, Anl64Role::FfnRmsNorm, Anl64Executor::Safe, 0u, 0u, NONE);
        anl64_add_node(plan, l, Anl64Role::FfnGate, Anl64Executor::Q4FastFixed, H, FFN, Q4);
        anl64_add_node(plan, l, Anl64Role::FfnUp, Anl64Executor::Q4FastFixed, H, FFN, Q4);
        anl64_add_node(plan, l, Anl64Role::SwiGlu, Anl64Executor::Safe, 0u, 0u, NONE);
        anl64_add_node(plan, l, Anl64Role::FfnDown, Anl64Executor::QuantSafe, FFN, H, uint16_t(down_types[l]));
        anl64_add_node(plan, l, Anl64Role::FfnResidual, Anl64Executor::Safe, 0u, 0u, NONE);
    }

    anl64_add_node(plan, 0xffffffffu, Anl64Role::OutputNorm, Anl64Executor::Safe, 0u, 0u, NONE);

    for (uint32_t rs = 0; rs < VOC; rs += LM_CHUNK) {
        const uint32_t rc = (rs + LM_CHUNK <= VOC) ? LM_CHUNK : (VOC - rs);
        anl64_add_node(plan, 0xffffffffu, Anl64Role::LmHead, Anl64Executor::QuantSafe, H, rc, Q6);
    }

    plan.metadata_bytes =
        uint64_t(plan.nodes.size()) * sizeof(Anl64PlanNode) +
        uint64_t(plan.regions.size()) * sizeof(Anl64Region64);

    if (plan.nodes.size() != 469u)
        throw std::runtime_error("ANL64 PlanNode census mismatch");
    if (plan.quant_linear_nodes != 215u)
        throw std::runtime_error("ANL64 quant-linear PlanNode census mismatch");
    if (plan.fixed_q4_fast_nodes != 140u)
        throw std::runtime_error("ANL64 fixed Q4_FAST PlanNode census mismatch");
    if (plan.regions.size() != 24104u)
        throw std::runtime_error("ANL64 Region64 census mismatch");
    if (plan.fixed_q4_regions != 19936u)
        throw std::runtime_error("ANL64 fixed Q4 Region64 census mismatch");
    if (plan.metadata_bytes > 2097152ull)
        throw std::runtime_error("ANL64 static plan metadata exceeds 2 MiB hard budget");

    anl64_hash_plan(plan);
    return plan;
}
