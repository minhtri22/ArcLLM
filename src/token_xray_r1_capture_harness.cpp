#include "../include/arcllm/v1/runtime.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint32_t> parse_tokens(const std::string& csv) {
    std::vector<std::uint32_t> out;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) throw std::runtime_error("empty token in --tokens");
        const auto v = std::stoull(item);
        if (v > 0xffffffffull) throw std::runtime_error("token exceeds uint32");
        out.push_back(static_cast<std::uint32_t>(v));
    }
    if (out.empty()) throw std::runtime_error("no input tokens");
    return out;
}

std::string esc(const std::string& s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        if (c == '\\' || c == '"') { o << '\\' << char(c); }
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << char(c);
    }
    return o.str();
}

void write_u32s(std::ostream& o, const std::vector<std::uint32_t>& v) {
    o << "[";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) o << ",";
        o << v[i];
    }
    o << "]";
}
}

int main(int argc, char** argv) {
    try {
        std::string model, shader_dir, sidecar, token_csv, mode, prompt_id, out_path, arcllm_head, model_sha256;
        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto need = [&](const char* flag) {
                if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + flag);
                return std::string(argv[++i]);
            };
            if (a == "--model") model = need("--model");
            else if (a == "--shader-dir") shader_dir = need("--shader-dir");
            else if (a == "--sidecar") sidecar = need("--sidecar");
            else if (a == "--tokens") token_csv = need("--tokens");
            else if (a == "--mode") mode = need("--mode");
            else if (a == "--prompt-id") prompt_id = need("--prompt-id");
            else if (a == "--out") out_path = need("--out");
            else if (a == "--arcllm-head") arcllm_head = need("--arcllm-head");
            else if (a == "--model-sha256") model_sha256 = need("--model-sha256");
            else throw std::runtime_error("unknown argument: " + a);
        }
        if (model.empty() || shader_dir.empty() || token_csv.empty() || prompt_id.empty() || out_path.empty() || arcllm_head.empty() || model_sha256.empty())
            throw std::runtime_error("required: --model --shader-dir --tokens --mode --prompt-id --out --arcllm-head --model-sha256");
        if (mode != "baseline" && mode != "instrumented")
            throw std::runtime_error("--mode must be baseline or instrumented");

        arcllm::v1::runtime::RunRequest req;
        req.model_path = model;
        req.shader_dir = shader_dir;
        req.sidecar_path = sidecar;
        req.input_token_ids = parse_tokens(token_csv);
        req.max_new_tokens = 4;
        req.evidence_profile = arcllm::v1::runtime::EvidenceProfile::PROFILE_0;
        req.request_within_validated_domain = false;
        req.capture_representation_trajectory = (mode == "instrumented");

        const auto t0 = std::chrono::steady_clock::now();
        const auto result = arcllm::v1::runtime::generate(req);
        const auto t1 = std::chrono::steady_clock::now();
        const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

        std::ofstream o(out_path, std::ios::binary | std::ios::trunc);
        if (!o) throw std::runtime_error("cannot open output");
        o << std::setprecision(std::numeric_limits<float>::max_digits10);
        o << "{\n";
        o << "  \"schema\":\"arcllm.token_xray_r1.single_request.v0.1\",\n";
        o << "  \"status\":\"COMPLETE\",\n";
        o << "  \"prompt_id\":\"" << esc(prompt_id) << "\",\n";
        o << "  \"mode\":\"" << mode << "\",\n";
        o << "  \"input_token_ids\":"; write_u32s(o, req.input_token_ids); o << ",\n";
        o << "  \"generated_token_ids\":"; write_u32s(o, result.generated_token_ids); o << ",\n";
        o << "  \"requested_decode_tokens\":4,\n";
        o << "  \"arcllm_head\":\"" << esc(arcllm_head) << "\",\n";
        o << "  \"model_sha256\":\"" << esc(model_sha256) << "\",\n";
        o << "  \"execution_domain_id\":\"gpu.arc_140v\",\n";
        o << "  \"evidence_profile\":\"PROFILE_0\",\n";
        o << "  \"request_within_validated_domain\":false,\n";
        o << "  \"request_elapsed_ns\":" << elapsed_ns << ",\n";
        o << "  \"runtime\":{";
        o << "\"finite\":" << (result.stats.finite ? "true" : "false");
        o << ",\"prefill_dispatches\":" << result.stats.prefill_dispatches;
        o << ",\"prefill_submits\":" << result.stats.prefill_submits;
        o << ",\"decode_dispatches_per_step\":" << result.stats.decode_dispatches_per_step;
        o << ",\"decode_submits_per_step\":" << result.stats.decode_submits_per_step;
        o << ",\"decode_steps\":" << result.stats.decode_steps;
        o << ",\"representation_prefill_capture_dispatches\":" << result.stats.representation_prefill_capture_dispatches;
        o << ",\"representation_decode_capture_dispatches\":" << result.stats.representation_decode_capture_dispatches;
        o << "},\n";
        o << "  \"capture_enabled\":" << (req.capture_representation_trajectory ? "true" : "false") << ",\n";
        o << "  \"representation_states\":[";
        for (std::size_t si = 0; si < result.representation_states.size(); ++si) {
            const auto& s = result.representation_states[si];
            if (si) o << ",";
            o << "\n    {";
            o << "\"phase\":\"" << esc(s.phase) << "\"";
            o << ",\"state_point\":\"" << esc(s.state_point) << "\"";
            o << ",\"state_index\":" << s.state_index;
            o << ",\"layer_index\":" << s.layer_index;
            o << ",\"token_id\":" << s.token_id;
            o << ",\"token_position\":" << s.token_position;
            o << ",\"hidden_dimension\":" << s.hidden_dimension;
            o << ",\"source_dtype\":\"" << esc(s.source_dtype) << "\"";
            o << ",\"values\":[";
            for (std::size_t i = 0; i < s.values.size(); ++i) {
                if (i) o << ",";
                o << s.values[i];
            }
            o << "]}";
        }
        if (!result.representation_states.empty()) o << "\n  ";
        o << "]\n";
        o << "}\n";
        o.close();

        std::cout << "TOKEN_XRAY_R1_REQUEST_COMPLETE mode=" << mode
                  << " prompt=" << prompt_id
                  << " generated=" << result.generated_token_ids.size()
                  << " states=" << result.representation_states.size()
                  << " elapsed_ns=" << elapsed_ns << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Token-XRay R1 harness error: " << e.what() << "\n";
        return 2;
    }
}
