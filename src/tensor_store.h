#pragma once
#include "gguf.h"
#include <cstdint>
#include <string>
#include <vector>

struct TensorSpan {
    std::string name;
    std::vector<uint64_t> dims;
    uint32_t ggml_type = 0;
    std::string ggml_type_name;
    uint64_t relative_offset = 0;
    uint64_t absolute_offset = 0;
    uint64_t nbytes = 0;
    uint64_t end_offset = 0;
    std::string sample_fingerprint;
};

struct TensorStoreReport {
    bool mapped = false;
    bool direct_zero_copy_view = false;
    bool all_bounds_valid = false;
    bool no_overlap = false;
    bool supported_types_only = false;
    bool q4_k_direct_access = false;

    uint64_t file_size = 0;
    uint64_t data_offset = 0;
    uint64_t mapped_file_bytes = 0;
    uint64_t tensor_bytes_total = 0;
    uint64_t q4_k_tensor_count = 0;
    uint64_t q6_k_tensor_count = 0;
    uint64_t f32_tensor_count = 0;

    std::vector<TensorSpan> tensors;
    std::string error;
};

class TensorStore {
public:
    TensorStore();
    ~TensorStore();

    TensorStore(const TensorStore&) = delete;
    TensorStore& operator=(const TensorStore&) = delete;

    TensorStoreReport inspect(const std::string& path, const GgufInfo& gguf);
    const uint8_t* mapped_base() const { return view_; }
    uint64_t mapped_size() const { return file_size_; }

private:
    void* file_ = nullptr;
    void* mapping_ = nullptr;
    const uint8_t* view_ = nullptr;
    uint64_t file_size_ = 0;

    void close();
};
