#pragma once

#include <array>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace token_xray {

inline std::string arcllm_v1_basename(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

inline std::string arcllm_v1_semantic_node_id(const std::string& name) {
    if (name == "token_embedding") return "decode.embedding";
    if (name == "output_norm") return "decode.output.norm";
    if (name == "lm_head") return "decode.output.lm_head";
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
    const std::string s = name.substr(dot + 1);
    if (s == "attn_rmsnorm") return "decode.layer." + l + ".attn.rmsnorm";
    if (s == "q_proj") return "decode.layer." + l + ".attn.q_proj";
    if (s == "k_proj") return "decode.layer." + l + ".attn.k_proj";
    if (s == "v_proj") return "decode.layer." + l + ".attn.v_proj";
    if (s == "q_rope") return "decode.layer." + l + ".attn.q_rope";
    if (s == "k_rope") return "decode.layer." + l + ".attn.k_rope";
    if (s == "kv_store") return "decode.layer." + l + ".attn.kv_update";
    if (s == "cached_gqa") return "decode.layer." + l + ".attn.core";
    if (s == "o_proj") return "decode.layer." + l + ".attn.o_proj";
    if (s == "attn_residual") return "decode.layer." + l + ".attn.residual";
    if (s == "ffn_rmsnorm") return "decode.layer." + l + ".ffn.rmsnorm";
    if (s == "ffn_gate") return "decode.layer." + l + ".ffn.gate";
    if (s == "ffn_up") return "decode.layer." + l + ".ffn.up";
    if (s == "swiglu") return "decode.layer." + l + ".ffn.activation";
    if (s == "ffn_down") return "decode.layer." + l + ".ffn.down";
    if (s == "ffn_residual") return "decode.layer." + l + ".ffn.residual";
    throw std::runtime_error("Token X-Ray ArcLLM adapter: unknown suffix " + s);
}

inline std::array<uint32_t,3> arcllm_v1_local_size(const std::string& path) {
    const auto s = arcllm_v1_basename(path);
    if (s == "p8c_embedding_q4k_segmented_probe.spv") return {256,1,1};
    if (s == "p7_rmsnorm_seq.spv") return {256,1,1};
    if (s == "p7_q4k_gemm_2d.spv" || s == "p7_q6k_gemm_2d.spv") return {64,1,1};
    if (s == "p7_rope_seq.spv") return {128,1,1};
    if (s == "p7_kv_store.spv") return {256,1,1};
    if (s == "p7_attention_kv_online.spv") return {128,1,1};
    if (s == "p7_add.spv") return {256,1,1};
    if (s == "sa1_q4k_subgroup_splitk.spv") return {128,1,1};
    if (s == "p7_swiglu.spv") return {256,1,1};
    if (s == "p8q1_lmhead_q6k_segmented_chunk.spv") return {64,1,1};
    throw std::runtime_error("Token X-Ray ArcLLM adapter: unknown shader " + s);
}

inline uint32_t arcllm_v1_subgroup_size(const std::string& path) {
    return arcllm_v1_basename(path) == "sa1_q4k_subgroup_splitk.spv" ? 32u : 0u;
}

} // namespace token_xray
