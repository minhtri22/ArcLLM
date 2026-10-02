#include "llama.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string json_escape(const std::string& s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        if (c == '\\' || c == '"') { o << '\\' << char(c); }
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (c < 0x20) {
            static const char* h = "0123456789abcdef";
            o << "\\u00" << h[(c >> 4) & 0xf] << h[c & 0xf];
        } else o << char(c);
    }
    return o.str();
}

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

std::string decode_hex(const std::string& h) {
    if (h.empty() || (h.size() % 2u) != 0u) throw std::runtime_error("invalid prompt hex length");
    std::string out;
    out.reserve(h.size() / 2u);
    for (std::size_t i = 0; i < h.size(); i += 2u) {
        const int hi = hex_digit(h[i]);
        const int lo = hex_digit(h[i + 1u]);
        if (hi < 0 || lo < 0) throw std::runtime_error("invalid prompt hex digit");
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out;
}

std::vector<llama_token> tokenize(const llama_vocab* vocab, const std::string& bytes) {
    std::vector<llama_token> out(bytes.size() + 32u);
    int32_t n = llama_tokenize(
        vocab,
        bytes.data(),
        static_cast<int32_t>(bytes.size()),
        out.data(),
        static_cast<int32_t>(out.size()),
        false,
        true
    );
    if (n < 0) {
        out.resize(static_cast<std::size_t>(-n));
        n = llama_tokenize(
            vocab,
            bytes.data(),
            static_cast<int32_t>(bytes.size()),
            out.data(),
            static_cast<int32_t>(out.size()),
            false,
            true
        );
    }
    if (n < 0) throw std::runtime_error("llama_tokenize failed");
    out.resize(static_cast<std::size_t>(n));
    return out;
}

struct PromptArg {
    std::string id;
    std::string hex;
};

PromptArg parse_prompt_arg(const std::string& v) {
    const auto pos = v.find('=');
    if (pos == std::string::npos || pos == 0u || pos + 1u >= v.size())
        throw std::runtime_error("--prompt requires ID=HEX");
    return PromptArg{v.substr(0u, pos), v.substr(pos + 1u)};
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::string model, out_path;
        std::vector<PromptArg> prompts;
        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto need = [&](const char* flag) {
                if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + flag);
                return std::string(argv[++i]);
            };
            if (a == "--model") model = need("--model");
            else if (a == "--out") out_path = need("--out");
            else if (a == "--prompt") prompts.push_back(parse_prompt_arg(need("--prompt")));
            else throw std::runtime_error("unknown argument: " + a);
        }
        if (model.empty() || out_path.empty() || prompts.empty())
            throw std::runtime_error("required: --model --out --prompt ID=HEX");

        llama_backend_init();
        llama_model_params mp = llama_model_default_params();
        mp.n_gpu_layers = 0;
        mp.vocab_only = true;
        llama_model* m = llama_model_load_from_file(model.c_str(), mp);
        if (!m) throw std::runtime_error("llama_model_load_from_file failed");
        const llama_vocab* vocab = llama_model_get_vocab(m);
        if (!vocab) {
            llama_model_free(m);
            llama_backend_free();
            throw std::runtime_error("llama_model_get_vocab failed");
        }

        std::ofstream o(out_path, std::ios::binary | std::ios::trunc);
        if (!o) {
            llama_model_free(m);
            llama_backend_free();
            throw std::runtime_error("cannot open output");
        }
        o << "{\n";
        o << "  \"schema\":\"arcllm.token_xray_r1r.exact_gguf_tokenization.v0.1\",\n";
        o << "  \"tokenizer_source\":\"exact_loaded_gguf_vocab\",\n";
        o << "  \"prompt_source\":\"hex_decoded_bytes\",\n";
        o << "  \"add_special_tokens\":false,\n";
        o << "  \"parse_special\":true,\n";
        o << "  \"prompts\":{\n";
        for (std::size_t pi = 0; pi < prompts.size(); ++pi) {
            const auto& p = prompts[pi];
            const std::string bytes = decode_hex(p.hex);
            const auto ids = tokenize(vocab, bytes);
            o << "    \"" << json_escape(p.id) << "\":{";
            o << "\"utf8_hex\":\"" << json_escape(p.hex) << "\",";
            o << "\"byte_count\":" << bytes.size() << ",";
            o << "\"token_ids\":[";
            for (std::size_t ti = 0; ti < ids.size(); ++ti) {
                if (ti) o << ",";
                o << ids[ti];
            }
            o << "],\"token_count\":" << ids.size() << "}";
            if (pi + 1u != prompts.size()) o << ",";
            o << "\n";
        }
        o << "  },\n";
        o << "  \"model_inference_executed\":false\n";
        o << "}\n";
        o.close();

        llama_model_free(m);
        llama_backend_free();
        std::cout << "TOKEN_XRAY_R1R_EXACT_GGUF_TOKENIZATION=COMPLETE\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "R1R tokenizer probe error: " << e.what() << "\n";
        return 2;
    }
}
