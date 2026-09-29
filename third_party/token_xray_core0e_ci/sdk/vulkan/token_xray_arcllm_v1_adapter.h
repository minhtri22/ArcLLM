#pragma once

#include <array>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace token_xray {

inline std::string arcllm_v1_basename(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

inline std::string arcllm_v1_phase_prefix(const std::string& mode) {
    if (mode == "decode" || mode == "prefill") return mode;
    throw std::runtime_error("Token X-Ray ArcLLM adapter: unsupported mode " + mode);
}

inline bool arcllm_v1_is_q4v4_p1_lifecycle_name(const std::string& name) {
    const std::string prefix = "Q4V4.P1.L";
    if (name.rfind(prefix, 0) != 0 || name.size() == prefix.size()) return false;
    for (size_t i = prefix.size(); i < name.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return false;
    }
    return true;
}

inline std::vector<std::string> arcllm_v1_semantic_node_ids(
    const std::string& name,
    const std::string& mode = "decode") {

    const std::string phase = arcllm_v1_phase_prefix(mode);

    if (arcllm_v1_is_q4v4_p1_lifecycle_name(name)) {
        throw std::runtime_error(
            "Token X-Ray ArcLLM adapter: Q4V4 lifecycle op is not model-semantic " + name);
    }

    if (name == "token_embedding") return {phase + ".embedding"};
    if (name == "output_norm") return {phase + ".output.norm"};
    if (name == "lm_head") return {phase + ".output.lm_head"};

    const auto dot = name.find('.');
    if (name.empty() || name[0] != 'L' || dot == std::string::npos || dot <= 1) {
        throw std::runtime_error("Token X-Ray ArcLLM adapter: malformed op " + name);
    }
    for (size_t i = 1; i < dot; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) {
            throw std::runtime_error("Token X-Ray ArcLLM adapter: malformed layer " + name);
        }
    }

    const int layer = std::stoi(name.substr(1, dot - 1));
    const std::string l = layer < 10 ? "0" + std::to_string(layer) : std::to_string(layer);
    const std::string base = phase + ".layer." + l;
    const std::string s = name.substr(dot + 1);

    if (s == "attn_rmsnorm") return {base + ".attn.rmsnorm"};
    if (s == "q_proj") return {base + ".attn.q_proj"};
    if (s == "k_proj") return {base + ".attn.k_proj"};
    if (s == "v_proj") return {base + ".attn.v_proj"};
    if (s == "q_rope") return {base + ".attn.q_rope"};
    if (s == "k_rope") return {base + ".attn.k_rope"};
    if (s == "kv_store") return {base + ".attn.kv_update"};
    if (s == "cached_gqa") return {base + ".attn.core"};
    if (s == "causal_gqa") {
        if (mode != "prefill") {
            throw std::runtime_error("Token X-Ray ArcLLM adapter: causal_gqa is prefill-only");
        }
        return {base + ".attn.core"};
    }
    if (s == "o_proj") return {base + ".attn.o_proj"};
    if (s == "attn_residual") return {base + ".attn.residual"};
    if (s == "ffn_rmsnorm") return {base + ".ffn.rmsnorm"};
    if (s == "ffn_gate") return {base + ".ffn.gate"};
    if (s == "ffn_up") return {base + ".ffn.up"};
    if (s == "ffn_gate_up_fused") {
        if (mode != "prefill") {
            throw std::runtime_error("Token X-Ray ArcLLM adapter: ffn_gate_up_fused is prefill-only");
        }
        return {base + ".ffn.gate", base + ".ffn.up"};
    }
    if (s == "swiglu") return {base + ".ffn.activation"};
    if (s == "ffn_down") return {base + ".ffn.down"};
    if (s == "ffn_residual") return {base + ".ffn.residual"};

    throw std::runtime_error("Token X-Ray ArcLLM adapter: unknown suffix " + s);
}

inline std::string arcllm_v1_semantic_node_id(
    const std::string& name,
    const std::string& mode = "decode") {
    const auto ids = arcllm_v1_semantic_node_ids(name, mode);
    if (ids.size() != 1) {
        throw std::runtime_error(
            "Token X-Ray ArcLLM adapter: op maps to multiple semantic nodes; use arcllm_v1_semantic_node_ids");
    }
    return ids.front();
}

struct ArcLLMV1LifecycleEvent {
    std::string event_type;
    std::string runtime_name;
    std::string representation_id;
    std::string resource_id;
    uint32_t layer = 0;
    std::string execution_domain_id;
};

inline ArcLLMV1LifecycleEvent arcllm_v1_q4v4_lifecycle_event(const std::string& name) {
    if (!arcllm_v1_is_q4v4_p1_lifecycle_name(name)) {
        throw std::runtime_error("Token X-Ray ArcLLM adapter: unsupported lifecycle op " + name);
    }
    const std::string prefix = "Q4V4.P1.L";
    const uint32_t layer = static_cast<uint32_t>(std::stoul(name.substr(prefix.size())));
    const std::string l = layer < 10 ? "0" + std::to_string(layer) : std::to_string(layer);
    return {
        "representation_materialize",
        name,
        "q4v4.exec148",
        "q4v4.exec148.layer." + l,
        layer,
        "gpu.arc_140v"
    };
}

inline std::array<uint32_t,3> arcllm_v1_local_size(const std::string& path) {
    const auto s = arcllm_v1_basename(path);
    if (s == "p8c_embedding_q4k_segmented_probe.spv") return {256,1,1};
    if (s == "p7_rmsnorm_seq.spv") return {256,1,1};
    if (s == "p7_q4k_gemm_2d.spv" || s == "p7_q6k_gemm_2d.spv") return {64,1,1};
    if (s == "q4_down_exec148_serial.spv") return {64,1,1};
    if (s == "p7_rope_seq.spv") return {128,1,1};
    if (s == "p7_kv_store.spv") return {256,1,1};
    if (s == "p7_attention_kv_online.spv" || s == "p7_attention_prefill_online.spv") return {128,1,1};
    if (s == "p7_add.spv") return {256,1,1};
    if (s == "sa1_q4k_subgroup_splitk.spv") return {128,1,1};
    if (s == "p7_swiglu.spv") return {256,1,1};
    if (s == "p7c_ffn_q4k_tiled.spv" || s == "p7c_ffn_q6k_tiled.spv" ||
        s == "p7l_ffn_q4k_gateup_fused.spv" ||
        s == "p7g_ffn_q4k_tiled16.spv" || s == "p7g_ffn_q6k_tiled16.spv") {
        return {8,8,1};
    }
    if (s == "p8q1_lmhead_q6k_segmented_chunk.spv") return {64,1,1};
    if (s == "b1_2_exec148_gpu_materialize.comp.spv") return {256,1,1};
    throw std::runtime_error("Token X-Ray ArcLLM adapter: unknown shader " + s);
}

inline uint32_t arcllm_v1_subgroup_size(const std::string& path) {
    return arcllm_v1_basename(path) == "sa1_q4k_subgroup_splitk.spv" ? 32u : 0u;
}

} // namespace token_xray
