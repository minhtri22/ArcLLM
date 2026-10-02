#include "llama.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::string esc(const std::string& s) {
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

static std::vector<llama_token> tokenize(const llama_vocab* vocab, const std::string& text) {
    std::vector<llama_token> out(text.size() + 16u);
    int32_t n = llama_tokenize(
        vocab,
        text.data(),
        static_cast<int32_t>(text.size()),
        out.data(),
        static_cast<int32_t>(out.size()),
        false,
        true
    );
    if (n < 0) {
        out.resize(static_cast<std::size_t>(-n));
        n = llama_tokenize(
            vocab,
            text.data(),
            static_cast<int32_t>(text.size()),
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

int main(int argc, char** argv) {
    std::string model, out_path;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto need = [&](const char* flag) {
                if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + flag);
                return std::string(argv[++i]);
            };
            if (a == "--model") model = need("--model");
            else if (a == "--out") out_path = need("--out");
            else throw std::runtime_error("unknown argument: " + a);
        }
        if (model.empty() || out_path.empty()) throw std::runtime_error("required: --model --out");

        llama_backend_init();
        llama_model_params mp = llama_model_default_params();
        mp.n_gpu_layers = 0;
        mp.vocab_only = true;
        mp.use_mmap = true;
        llama_model* m = llama_model_load_from_file(model.c_str(), mp);
        if (!m) throw std::runtime_error("llama_model_load_from_file failed");
        const llama_vocab* vocab = llama_model_get_vocab(m);
        if (!vocab) {
            llama_model_free(m);
            llama_backend_free();
            throw std::runtime_error("llama_model_get_vocab failed");
        }

        struct Prompt { const char* id; const char* text; };
        const Prompt prompts[] = {
            {"P0", "The capital of France is"},
            {"P1", "2 + 2 ="},
            {"P2", u8"Hà Nội là thủ đô của"},
            {"P3", "A B C D E F G H"},
        };

        std::ofstream o(out_path, std::ios::binary | std::ios::trunc);
        if (!o) {
            llama_model_free(m);
            llama_backend_free();
            throw std::runtime_error("cannot open output");
        }
        o << "{\n  \"schema\":\"arcllm.token_xray_r1.exact_gguf_tokenization.v0.1\",\n";
        o << "  \"status\":\"PASS_EXACT_GGUF_TOKENIZATION\",\n";
        o << "  \"tokenizer_source\":\"exact_loaded_gguf_vocab\",\n";
        o << "  \"add_special_tokens\":false,\n";
        o << "  \"parse_special\":true,\n";
        o << "  \"prompts\":{\n";
        for (std::size_t pi = 0; pi < 4; ++pi) {
            const std::string text = prompts[pi].text;
            const auto ids = tokenize(vocab, text);
            o << "    \"" << prompts[pi].id << "\":{\"utf8\":\"" << esc(text) << "\",\"token_ids\":[";
            for (std::size_t i = 0; i < ids.size(); ++i) {
                if (i) o << ",";
                o << ids[i];
            }
            o << "],\"token_count\":" << ids.size() << "}";
            if (pi + 1 != 4) o << ",";
            o << "\n";
        }
        o << "  },\n";
        o << "  \"model_inference_executed\":false\n}\n";
        o.close();
        llama_model_free(m);
        llama_backend_free();
        std::cout << "TOKEN_XRAY_R1_EXACT_GGUF_TOKENIZATION=PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "R1 tokenizer probe error: " << e.what() << "\n";
        return 2;
    }
}
