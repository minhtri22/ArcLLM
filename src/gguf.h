#pragma once
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <vector>

struct GgufTensorInfo {
    std::string name;
    uint32_t n_dims = 0;
    std::vector<uint64_t> dims;
    uint32_t ggml_type = 0;
    uint64_t offset = 0;
};

struct GgufInfo {
    uint32_t version = 0;
    uint64_t tensor_count = 0;
    uint64_t metadata_count = 0;
    uint64_t alignment = 32;
    uint64_t data_offset = 0;

    std::map<std::string, std::string> scalars;
    std::map<std::string, uint64_t> array_lengths;
    std::map<uint32_t, uint64_t> tensor_type_counts;
    std::vector<GgufTensorInfo> tensors;
};

class GgufReader {
public:
    explicit GgufReader(const std::string& path);
    GgufInfo read();

private:
    std::ifstream f_;

    template<typename T> T read_pod() {
        T v{};
        f_.read(reinterpret_cast<char*>(&v), sizeof(T));
        if (!f_) throw std::runtime_error("unexpected EOF");
        return v;
    }

    std::string read_string();
    std::string read_value_as_string(uint32_t type, uint64_t* array_len = nullptr);
    void skip_value(uint32_t type);
};
