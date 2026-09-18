#include "gguf.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
enum : uint32_t {
    GGUF_TYPE_UINT8   = 0,
    GGUF_TYPE_INT8    = 1,
    GGUF_TYPE_UINT16  = 2,
    GGUF_TYPE_INT16   = 3,
    GGUF_TYPE_UINT32  = 4,
    GGUF_TYPE_INT32   = 5,
    GGUF_TYPE_FLOAT32 = 6,
    GGUF_TYPE_BOOL    = 7,
    GGUF_TYPE_STRING  = 8,
    GGUF_TYPE_ARRAY   = 9,
    GGUF_TYPE_UINT64  = 10,
    GGUF_TYPE_INT64   = 11,
    GGUF_TYPE_FLOAT64 = 12
};

template<typename T>
std::string num_to_string(T v) {
    std::ostringstream o;
    o << v;
    return o.str();
}

std::string bool_string(uint8_t v) { return v ? "true" : "false"; }

uint64_t align_up(uint64_t x, uint64_t a) {
    if (!a) return x;
    return (x + a - 1) / a * a;
}
}

GgufReader::GgufReader(const std::string& path)
    : f_(path, std::ios::binary) {
    if (!f_) throw std::runtime_error("cannot open GGUF: " + path);
}

std::string GgufReader::read_string() {
    uint64_t n = read_pod<uint64_t>();
    if (n > (1ull << 31)) throw std::runtime_error("GGUF string too large");
    std::string s((size_t)n, '\0');
    if (n) {
        f_.read(s.data(), static_cast<std::streamsize>(n));
        if (!f_) throw std::runtime_error("unexpected EOF reading GGUF string");
    }
    return s;
}

void GgufReader::skip_value(uint32_t type) {
    switch (type) {
        case GGUF_TYPE_UINT8:   (void)read_pod<uint8_t>(); break;
        case GGUF_TYPE_INT8:    (void)read_pod<int8_t>(); break;
        case GGUF_TYPE_UINT16:  (void)read_pod<uint16_t>(); break;
        case GGUF_TYPE_INT16:   (void)read_pod<int16_t>(); break;
        case GGUF_TYPE_UINT32:  (void)read_pod<uint32_t>(); break;
        case GGUF_TYPE_INT32:   (void)read_pod<int32_t>(); break;
        case GGUF_TYPE_FLOAT32: (void)read_pod<float>(); break;
        case GGUF_TYPE_BOOL:    (void)read_pod<uint8_t>(); break;
        case GGUF_TYPE_STRING:  (void)read_string(); break;
        case GGUF_TYPE_UINT64:  (void)read_pod<uint64_t>(); break;
        case GGUF_TYPE_INT64:   (void)read_pod<int64_t>(); break;
        case GGUF_TYPE_FLOAT64: (void)read_pod<double>(); break;
        case GGUF_TYPE_ARRAY: {
            uint32_t elem_type = read_pod<uint32_t>();
            uint64_t n = read_pod<uint64_t>();
            if (n > (1ull << 32)) throw std::runtime_error("GGUF array too large");
            for (uint64_t i = 0; i < n; ++i) skip_value(elem_type);
            break;
        }
        default:
            throw std::runtime_error("unsupported GGUF metadata type: " + std::to_string(type));
    }
}

std::string GgufReader::read_value_as_string(uint32_t type, uint64_t* array_len) {
    switch (type) {
        case GGUF_TYPE_UINT8:   return num_to_string((uint32_t)read_pod<uint8_t>());
        case GGUF_TYPE_INT8:    return num_to_string((int32_t)read_pod<int8_t>());
        case GGUF_TYPE_UINT16:  return num_to_string(read_pod<uint16_t>());
        case GGUF_TYPE_INT16:   return num_to_string(read_pod<int16_t>());
        case GGUF_TYPE_UINT32:  return num_to_string(read_pod<uint32_t>());
        case GGUF_TYPE_INT32:   return num_to_string(read_pod<int32_t>());
        case GGUF_TYPE_FLOAT32: return num_to_string(read_pod<float>());
        case GGUF_TYPE_BOOL:    return bool_string(read_pod<uint8_t>());
        case GGUF_TYPE_STRING:  return read_string();
        case GGUF_TYPE_UINT64:  return num_to_string(read_pod<uint64_t>());
        case GGUF_TYPE_INT64:   return num_to_string(read_pod<int64_t>());
        case GGUF_TYPE_FLOAT64: return num_to_string(read_pod<double>());
        case GGUF_TYPE_ARRAY: {
            uint32_t elem_type = read_pod<uint32_t>();
            uint64_t n = read_pod<uint64_t>();
            if (array_len) *array_len = n;
            if (n > (1ull << 32)) throw std::runtime_error("GGUF array too large");
            for (uint64_t i = 0; i < n; ++i) skip_value(elem_type);
            return "<array type=" + std::to_string(elem_type) + " len=" + std::to_string(n) + ">";
        }
        default:
            throw std::runtime_error("unsupported GGUF metadata type: " + std::to_string(type));
    }
}

GgufInfo GgufReader::read() {
    char magic[4]{};
    f_.read(magic, 4);
    if (!f_ || std::memcmp(magic, "GGUF", 4) != 0)
        throw std::runtime_error("not a GGUF file");

    GgufInfo out;
    out.version = read_pod<uint32_t>();
    if (out.version < 2 || out.version > 3)
        throw std::runtime_error("unsupported GGUF version: " + std::to_string(out.version));

    out.tensor_count = read_pod<uint64_t>();
    out.metadata_count = read_pod<uint64_t>();

    for (uint64_t i = 0; i < out.metadata_count; ++i) {
        std::string key = read_string();
        uint32_t type = read_pod<uint32_t>();
        uint64_t array_len = 0;
        std::string val = read_value_as_string(type, &array_len);
        out.scalars[key] = val;
        if (type == GGUF_TYPE_ARRAY) out.array_lengths[key] = array_len;
        if (key == "general.alignment") {
            try { out.alignment = std::stoull(val); } catch (...) {}
        }
    }

    out.tensors.reserve((size_t)out.tensor_count);
    for (uint64_t i = 0; i < out.tensor_count; ++i) {
        GgufTensorInfo t;
        t.name = read_string();
        t.n_dims = read_pod<uint32_t>();
        if (t.n_dims > 8) throw std::runtime_error("unexpected tensor ndim > 8");
        for (uint32_t d = 0; d < t.n_dims; ++d)
            t.dims.push_back(read_pod<uint64_t>());
        t.ggml_type = read_pod<uint32_t>();
        t.offset = read_pod<uint64_t>();
        out.tensor_type_counts[t.ggml_type]++;
        out.tensors.push_back(std::move(t));
    }

    uint64_t pos = static_cast<uint64_t>(f_.tellg());
    out.data_offset = align_up(pos, out.alignment);
    return out;
}
