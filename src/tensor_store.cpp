#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "tensor_store.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

constexpr uint32_t GGML_TYPE_F32  = 0;
constexpr uint32_t GGML_TYPE_Q4_K = 12;
constexpr uint32_t GGML_TYPE_Q6_K = 14;

constexpr uint64_t QK_K = 256;
constexpr uint64_t TYPE_SIZE_Q4_K = 144; // block_q4_K
constexpr uint64_t TYPE_SIZE_Q6_K = 210; // block_q6_K

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(),
                                static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) throw std::runtime_error("UTF-8 path conversion failed");
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(),
                        static_cast<int>(s.size()), w.data(), n);
    return w;
}

uint64_t checked_mul(uint64_t a, uint64_t b) {
    if (a && b > (std::numeric_limits<uint64_t>::max)() / a)
        throw std::runtime_error("tensor size overflow");
    return a * b;
}

std::string type_name(uint32_t t) {
    switch (t) {
        case GGML_TYPE_F32:  return "F32";
        case GGML_TYPE_Q4_K: return "Q4_K";
        case GGML_TYPE_Q6_K: return "Q6_K";
        default: return "TYPE_" + std::to_string(t);
    }
}

uint64_t row_bytes(uint32_t type, uint64_t ne0) {
    switch (type) {
        case GGML_TYPE_F32:
            return checked_mul(ne0, 4);
        case GGML_TYPE_Q4_K:
            if (ne0 % QK_K != 0)
                throw std::runtime_error("Q4_K ne0 not divisible by 256");
            return checked_mul(ne0 / QK_K, TYPE_SIZE_Q4_K);
        case GGML_TYPE_Q6_K:
            if (ne0 % QK_K != 0)
                throw std::runtime_error("Q6_K ne0 not divisible by 256");
            return checked_mul(ne0 / QK_K, TYPE_SIZE_Q6_K);
        default:
            throw std::runtime_error("unsupported tensor type " + std::to_string(type));
    }
}

uint64_t tensor_nbytes(const GgufTensorInfo& t) {
    if (t.dims.empty()) return 0;
    uint64_t rows = 1;
    for (size_t i = 1; i < t.dims.size(); ++i)
        rows = checked_mul(rows, t.dims[i]);
    return checked_mul(row_bytes(t.ggml_type, t.dims[0]), rows);
}

uint64_t fnv1a_update(uint64_t h, const uint8_t* p, size_t n) {
    constexpr uint64_t prime = 1099511628211ull;
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= prime;
    }
    return h;
}

std::string sample_fingerprint(const uint8_t* p, uint64_t n) {
    constexpr uint64_t basis = 14695981039346656037ull;
    uint64_t h = basis;
    if (!n) {
        std::ostringstream o;
        o << std::hex << std::setw(16) << std::setfill('0') << h;
        return o.str();
    }
    size_t head = static_cast<size_t>(std::min<uint64_t>(64, n));
    h = fnv1a_update(h, p, head);
    if (n > head) {
        size_t tail = static_cast<size_t>(std::min<uint64_t>(64, n - head));
        h = fnv1a_update(h, p + (n - tail), tail);
    }
    std::ostringstream o;
    o << std::hex << std::setw(16) << std::setfill('0') << h;
    return o.str();
}

}

TensorStore::TensorStore() = default;
TensorStore::~TensorStore() { close(); }

void TensorStore::close() {
    if (view_) {
        UnmapViewOfFile(view_);
        view_ = nullptr;
    }
    if (mapping_) {
        CloseHandle(reinterpret_cast<HANDLE>(mapping_));
        mapping_ = nullptr;
    }
    if (file_) {
        CloseHandle(reinterpret_cast<HANDLE>(file_));
        file_ = nullptr;
    }
    file_size_ = 0;
}

TensorStoreReport TensorStore::inspect(const std::string& path, const GgufInfo& gguf) {
    close();
    TensorStoreReport out;

    try {
        std::wstring wpath = utf8_to_wide(path);
        HANDLE f = CreateFileW(
            wpath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (f == INVALID_HANDLE_VALUE)
            throw std::runtime_error("CreateFileW failed: " + std::to_string(GetLastError()));
        file_ = f;

        LARGE_INTEGER sz{};
        if (!GetFileSizeEx(f, &sz) || sz.QuadPart < 0)
            throw std::runtime_error("GetFileSizeEx failed");
        file_size_ = static_cast<uint64_t>(sz.QuadPart);
        out.file_size = file_size_;
        out.data_offset = gguf.data_offset;

        HANDLE m = CreateFileMappingW(f, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!m)
            throw std::runtime_error("CreateFileMappingW failed: " + std::to_string(GetLastError()));
        mapping_ = m;

        const uint8_t* view = reinterpret_cast<const uint8_t*>(
            MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0)
        );
        if (!view)
            throw std::runtime_error("MapViewOfFile failed: " + std::to_string(GetLastError()));
        view_ = view;

        out.mapped = true;
        out.direct_zero_copy_view = true;
        out.mapped_file_bytes = file_size_;

        bool bounds_ok = true;
        bool supported_only = true;
        uint64_t total = 0;

        out.tensors.reserve(gguf.tensors.size());

        for (const auto& t : gguf.tensors) {
            TensorSpan s;
            s.name = t.name;
            s.dims = t.dims;
            s.ggml_type = t.ggml_type;
            s.ggml_type_name = type_name(t.ggml_type);
            s.relative_offset = t.offset;
            s.absolute_offset = gguf.data_offset + t.offset;

            try {
                s.nbytes = tensor_nbytes(t);
            } catch (const std::exception&) {
                supported_only = false;
                throw;
            }

            if (s.absolute_offset > file_size_ ||
                s.nbytes > file_size_ - s.absolute_offset) {
                bounds_ok = false;
                throw std::runtime_error("tensor out of bounds: " + t.name);
            }

            s.end_offset = s.absolute_offset + s.nbytes;
            s.sample_fingerprint = sample_fingerprint(view_ + s.absolute_offset, s.nbytes);

            total = checked_mul(1, total + s.nbytes);

            if (t.ggml_type == GGML_TYPE_Q4_K) out.q4_k_tensor_count++;
            else if (t.ggml_type == GGML_TYPE_Q6_K) out.q6_k_tensor_count++;
            else if (t.ggml_type == GGML_TYPE_F32) out.f32_tensor_count++;
            else supported_only = false;

            out.tensors.push_back(std::move(s));
        }

        std::vector<const TensorSpan*> sorted;
        sorted.reserve(out.tensors.size());
        for (const auto& s : out.tensors) sorted.push_back(&s);
        std::sort(sorted.begin(), sorted.end(),
                  [](const TensorSpan* a, const TensorSpan* b) {
                      return a->absolute_offset < b->absolute_offset;
                  });

        bool no_overlap = true;
        uint64_t prev_end = gguf.data_offset;
        for (const auto* s : sorted) {
            if (s->absolute_offset < prev_end) {
                no_overlap = false;
                break;
            }
            prev_end = s->end_offset;
        }

        out.tensor_bytes_total = total;
        out.all_bounds_valid = bounds_ok;
        out.no_overlap = no_overlap;
        out.supported_types_only = supported_only;
        out.q4_k_direct_access =
            out.direct_zero_copy_view &&
            out.q4_k_tensor_count > 0 &&
            bounds_ok &&
            no_overlap;

        return out;

    } catch (const std::exception& e) {
        out.error = e.what();
        return out;
    }
}
