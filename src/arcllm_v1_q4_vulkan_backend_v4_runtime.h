#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/arcllm/v1/generic_policy_engine_v4.h"
#include "../include/arcllm/v1/generic_backend_binding_v4.h"
#include "registrations/arcllm_v1_q4k_down_reference_registration_v2.h"

#include "arcllm_v1_vulkan_runtime_support.h"

#include "q4_down_exec148_materializer.h"

namespace {


namespace reg = arcllm::v1::registry_v2;
namespace gp = arcllm::v1::policy_v4;
namespace bind = arcllm::v1::binding_v4;
namespace q4reg = arcllm::v1::reference_registration_v2;
using namespace arcllm_exec148;

static constexpr std::array<uint32_t,14> kQ4Layers={
    3u,4u,6u,7u,8u,11u,12u,14u,15u,17u,18u,19u,21u,22u
};
static constexpr uint32_t kFFN=18944u;
static constexpr uint32_t kHidden=3584u;
static constexpr uint32_t kBlocksPerLayer=kRows*kBlocksPerRow;
static constexpr uint32_t kP1LocalSize=256u;
static constexpr uint32_t kP1Groups=kBlocksPerLayer/kP1LocalSize;
static constexpr uint64_t kP3ChunkBytes=4ull*1024ull*1024ull;
static constexpr char kModelSha[]=
    "60e05f2100071479f596b964f89f510f057ce397ea22f2833a0cfe029bfc2463";
static constexpr char kCanonicalTupleSha[]=
    "60565f9f0b12de4884e884d8311263df7238679745c83393a695935cd3eccbb2";
static constexpr char kCanonicalRawSha[]=
    "3f168749256e8acbbbe61da06196ca0e51a249b3923938e5651a4da6f50ab43f";

struct ArenaRange{uint64_t start,end;};
static constexpr std::array<ArenaRange,19> kFrozenWeightArenas={{
    {0ull,268434432ull},{268434432ull,511383552ull},{511383552ull,753913856ull},
    {753913856ull,1016184832ull},{1016184832ull,1280919552ull},{1280919552ull,1538461696ull},
    {1538461696ull,1800732672ull},{1800732672ull,2042789888ull},{2042789888ull,2267342848ull},
    {2267342848ull,2531604480ull},{2531604480ull,2755108864ull},{2755108864ull,3018350592ull},
    {3018350592ull,3260407808ull},{3260407808ull,3484487680ull},{3484487680ull,3727486976ull},
    {3727486976ull,3987521536ull},{3987521536ull,4230051840ull},{4230051840ull,4498485600ull},
    {4498485600ull,4677120000ull}
}};

struct Q4PCMat {
    uint32_t src_base_bytes;
    uint32_t dst_base_bytes;
    uint32_t block_count;
    uint32_t reserved;
};
struct Q4PCGemm {
    uint32_t n,rows,batch,row_bytes,add_bias,w_base_bytes,bias_base;
};
struct ImageValidation {
    bool exact=false;
    uint64_t blocks=0;
    std::string source_tuple_sha;
    std::string exec_tuple_sha;
    std::string raw_sha;
};

struct BackendCounters {
    uint64_t b_allocations=0;
    uint64_t b_materializations=0;
    uint64_t b_validations=0;
    uint64_t b_releases=0;
    uint64_t p1_calls=0;
    uint64_t p3_calls=0;
    uint64_t p0_calls=0;
    uint64_t resolve_a=0;
    uint64_t resolve_b=0;
};

static std::string raw_sha256(const void* p,uint64_t n){
    Sha256 h;
    h.update(reinterpret_cast<const uint8_t*>(p),size_t(n));
    return Sha256::hex(h.final());
}

static ImageValidation validate_image(
    const std::array<const uint8_t*,14>& src,
    const uint8_t* exec){
    ImageValidation v;
    Sha256 hs,he;
    v.exact=true;
    for(uint32_t slot=0;slot<14u;++slot){
        auto r=validate_tensor(
            src[slot],
            exec+uint64_t(slot)*kExecLayerBytes,
            kRows,kSourceRowBytes,kExecRowBytes,&hs,&he);
        v.exact=v.exact&&r.exact&&r.blocks_checked==uint64_t(kRows)*kBlocksPerRow;
        v.blocks+=r.blocks_checked;
    }
    v.source_tuple_sha=Sha256::hex(hs.final());
    v.exec_tuple_sha=Sha256::hex(he.final());
    v.raw_sha=raw_sha256(exec,kExecFamilyBytes);
    v.exact=v.exact&&
        v.source_tuple_sha==kCanonicalTupleSha&&
        v.exec_tuple_sha==kCanonicalTupleSha&&
        v.raw_sha==kCanonicalRawSha;
    return v;
}

static std::vector<DispatchOp> build_p1_ops(
    const std::array<Buffer*,14>& src_buffers,
    const std::array<uint32_t,14>& src_base,
    Buffer& exec,
    const std::string& shader_dir){
    static_assert(kBlocksPerLayer%kP1LocalSize==0u,"P1 geometry mismatch");
    std::vector<DispatchOp> ops;
    ops.reserve(14u);
    for(uint32_t slot=0;slot<14u;++slot){
        Q4PCMat pc{
            src_base[slot],
            uint32_t(uint64_t(slot)*kExecLayerBytes),
            kBlocksPerLayer,
            0u
        };
        ops.push_back({
            std::string("Q4V4.P1.L")+std::to_string(kQ4Layers[slot]),
            join_path_p8c(shader_dir,"b1_2_exec148_gpu_materialize.comp.spv"),
            {src_buffers[slot],&exec},
            push_bytes(pc),
            kP1Groups,1u,1u
        });
    }
    return ops;
}

static uint32_t unbuffered_sector_bytes(const std::string& sidecar){
    std::wstring abs=std::filesystem::absolute(std::filesystem::path(sidecar)).wstring();
    wchar_t root[MAX_PATH]{};
    if(!GetVolumePathNameW(abs.c_str(),root,MAX_PATH))
        throw std::runtime_error("GetVolumePathNameW failed");
    DWORD spc=0,bps=0,freec=0,totalc=0;
    if(!GetDiskFreeSpaceW(root,&spc,&bps,&freec,&totalc)||bps==0u)
        throw std::runtime_error("GetDiskFreeSpaceW failed");
    if((4096u%bps)!=0u||(kP3ChunkBytes%bps)!=0u||(kExecFamilyBytes%bps)!=0u)
        throw std::runtime_error("P3 alignment incompatible with sector");
    return bps;
}

static void unbuffered_load(
    const std::string& sidecar,
    uint8_t* dst){
    const uint32_t sector=unbuffered_sector_bytes(sidecar);
    std::wstring path=std::filesystem::absolute(std::filesystem::path(sidecar)).wstring();
    HANDLE h=CreateFileW(
        path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(h==INVALID_HANDLE_VALUE)
        throw std::runtime_error("P3 CreateFileW failed");
    void* staging=VirtualAlloc(
        nullptr,size_t(kP3ChunkBytes),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!staging){
        CloseHandle(h);
        throw std::runtime_error("P3 VirtualAlloc failed");
    }
    try{
        uint64_t off=0;
        while(off<kExecFamilyBytes){
            const DWORD want=DWORD((std::min)(kP3ChunkBytes,kExecFamilyBytes-off));
            if((want%sector)!=0u)throw std::runtime_error("P3 unaligned read");
            DWORD got=0;
            if(!ReadFile(h,staging,want,&got,nullptr)||got!=want)
                throw std::runtime_error("P3 short read");
            std::memcpy(dst+off,staging,want);
            off+=want;
        }
    }catch(...){
        VirtualFree(staging,0,MEM_RELEASE);
        CloseHandle(h);
        throw;
    }
    VirtualFree(staging,0,MEM_RELEASE);
    CloseHandle(h);
}

class Q4VulkanBackendV4 final : public bind::BackendAdapter {
public:
    Q4VulkanBackendV4(
        VkRuntime& vk,
        const std::array<const uint8_t*,14>& src_cpu,
        const std::array<Buffer*,14>& src_gpu,
        const std::array<uint32_t,14>& src_base,
        std::string shader_dir,
        std::string sidecar)
        : vk_(vk),src_cpu_(src_cpu),src_gpu_(src_gpu),src_base_(src_base),
          shader_dir_(std::move(shader_dir)),sidecar_(std::move(sidecar)) {}

    ~Q4VulkanBackendV4() override {
        if(resident_)vk_.destroy_buffer(exec_);
    }

    bind::BackendStatus acquire(
        reg::AcquisitionPathId path,
        reg::PrimitiveId target,
        std::uint64_t residency_bytes,
        bind::RepresentationHandle& out) noexcept override {
        out={};
        if(target!=q4reg::kPrimitiveB||residency_bytes!=kExecFamilyBytes)
            return bind::BackendStatus::INVALID_BINDING_STATE;
        if(resident_)
            return bind::BackendStatus::INVALID_BINDING_STATE;
        if(path==q4reg::kAcquireSecondary){
            if(sidecar_.empty()||!std::filesystem::exists(sidecar_))
                return bind::BackendStatus::UNAVAILABLE;
            try{
                if(std::filesystem::file_size(sidecar_)!=kExecFamilyBytes)
                    return bind::BackendStatus::UNAVAILABLE;
            }catch(...){
                return bind::BackendStatus::UNAVAILABLE;
            }
        }
        if(path!=q4reg::kAcquirePrimary&&
           path!=q4reg::kAcquireSecondary&&
           path!=q4reg::kAcquireTertiary)
            return bind::BackendStatus::UNAVAILABLE;

        try{
            exec_=vk_.make_buffer(kExecFamilyBytes,nullptr);
            ++counters_.b_allocations;
            if(path==q4reg::kAcquirePrimary){
                ++counters_.p1_calls;
                auto ops=build_p1_ops(src_gpu_,src_base_,exec_,shader_dir_);
                if(ops.size()!=14u)throw std::runtime_error("P1 dispatch count mismatch");
                auto chain=vk_.prepare_chain(ops);
                (void)vk_.execute_prepared(chain,ops,false);
                vk_.destroy_prepared(chain);
            }else if(path==q4reg::kAcquireSecondary){
                ++counters_.p3_calls;
                unbuffered_load(sidecar_,reinterpret_cast<uint8_t*>(exec_.mapped));
            }else{
                ++counters_.p0_calls;
                for(uint32_t slot=0;slot<14u;++slot)
                    materialize_tensor(
                        src_cpu_[slot],
                        reinterpret_cast<uint8_t*>(exec_.mapped)+uint64_t(slot)*kExecLayerBytes);
            }
            ++counters_.b_materializations;
            resident_=true;
            validated_=false;
            out={1ull};
            return bind::BackendStatus::OK;
        }catch(const std::exception& e){
            last_error_=e.what();
            if(exec_.buffer||exec_.memory||exec_.mapped)vk_.destroy_buffer(exec_);
            resident_=false;
            validated_=false;
            return bind::BackendStatus::ACQUISITION_FAILED;
        }catch(...){
            last_error_="unknown acquisition failure";
            if(exec_.buffer||exec_.memory||exec_.mapped)vk_.destroy_buffer(exec_);
            resident_=false;
            validated_=false;
            return bind::BackendStatus::ACQUISITION_FAILED;
        }
    }

    bind::BackendStatus validate(
        bind::RepresentationHandle representation,
        reg::CapabilityId capability,
        reg::PrimitiveId target) noexcept override {
        if(representation.opaque!=1ull||
           capability!=q4reg::kCapability||
           target!=q4reg::kPrimitiveB||
           !resident_)
            return bind::BackendStatus::INVALID_BINDING_STATE;
        try{
            ++counters_.b_validations;
            const ImageValidation v=validate_image(
                src_cpu_,reinterpret_cast<const uint8_t*>(exec_.mapped));
            last_validation_=v;
            if(!v.exact){
                validated_=false;
                return bind::BackendStatus::VALIDATION_FAILED;
            }
            validated_=true;
            return bind::BackendStatus::OK;
        }catch(const std::exception& e){
            last_error_=e.what();
            validated_=false;
            return bind::BackendStatus::VALIDATION_FAILED;
        }catch(...){
            last_error_="unknown validation failure";
            validated_=false;
            return bind::BackendStatus::VALIDATION_FAILED;
        }
    }

    bind::BackendStatus release(
        reg::PrimitiveId target,
        bind::RepresentationHandle representation) noexcept override {
        if(target!=q4reg::kPrimitiveB||representation.opaque!=1ull||!resident_)
            return bind::BackendStatus::INVALID_BINDING_STATE;
        try{
            vk_.destroy_buffer(exec_);
            resident_=false;
            validated_=false;
            ++counters_.b_releases;
            return bind::BackendStatus::OK;
        }catch(...){
            return bind::BackendStatus::RELEASE_FAILED;
        }
    }

    bind::BackendStatus resolve_primitive(
        reg::PrimitiveId primitive,
        bind::PrimitiveHandle& out) noexcept override {
        out={};
        if(primitive==q4reg::kPrimitiveA){
            ++counters_.resolve_a;
            const auto p=std::filesystem::path(shader_dir_)/"sa1_q4k_subgroup_splitk.spv";
            if(!std::filesystem::exists(p))
                return bind::BackendStatus::PRIMITIVE_UNAVAILABLE;
            out={primitive.value};
            return bind::BackendStatus::OK;
        }
        if(primitive==q4reg::kPrimitiveB){
            ++counters_.resolve_b;
            const auto p=std::filesystem::path(shader_dir_)/"q4_down_exec148_serial.spv";
            if(!resident_||!validated_||!std::filesystem::exists(p))
                return bind::BackendStatus::PRIMITIVE_UNAVAILABLE;
            out={primitive.value};
            return bind::BackendStatus::OK;
        }
        return bind::BackendStatus::PRIMITIVE_UNAVAILABLE;
    }

    void append_ffn_down_op(
        std::vector<DispatchOp>& ops,
        const std::string& name,
        bind::PrimitiveHandle primitive,
        uint32_t layer,
        Buffer* source_buffer,
        uint32_t source_base_bytes,
        Buffer& input,
        Buffer& dummy,
        Buffer& output){
        if(primitive.opaque==q4reg::kPrimitiveA.value){
            ops.push_back({
                name,
                join_path_p8c(shader_dir_,"sa1_q4k_subgroup_splitk.spv"),
                {source_buffer,&input,&dummy,&output},
                push_bytes(Q4PCGemm{
                    kFFN,kHidden,1u,kSourceRowBytes,0u,source_base_bytes,0u}),
                896u,1u,1u
            });
            return;
        }
        if(primitive.opaque==q4reg::kPrimitiveB.value&&resident_&&validated_){
            uint32_t slot=uint32_t(kQ4Layers.size());
            for(uint32_t i=0;i<uint32_t(kQ4Layers.size());++i)
                if(kQ4Layers[i]==layer){slot=i;break;}
            if(slot==uint32_t(kQ4Layers.size()))
                throw std::runtime_error("B route requested for non-Q4-down frozen layer");
            ops.push_back({
                name,
                join_path_p8c(shader_dir_,"q4_down_exec148_serial.spv"),
                {&exec_,&input,&dummy,&output},
                push_bytes(Q4PCGemm{
                    kFFN,kHidden,1u,kExecRowBytes,0u,
                    uint32_t(uint64_t(slot)*kExecLayerBytes),0u}),
                56u,1u,1u
            });
            return;
        }
        throw std::runtime_error("Q4 backend primitive handle not routable");
    }

bool resident() const noexcept{return resident_;}
    bool validated() const noexcept{return validated_;}
    bool p3_available() const noexcept{
        try{
            return !sidecar_.empty()&&
                std::filesystem::exists(sidecar_)&&
                std::filesystem::file_size(sidecar_)==kExecFamilyBytes;
        }catch(...){return false;}
    }
    const BackendCounters& counters() const noexcept{return counters_;}
    const ImageValidation& last_validation() const noexcept{return last_validation_;}
    const std::string& last_error() const noexcept{return last_error_;}

private:
    VkRuntime& vk_;
    const std::array<const uint8_t*,14>& src_cpu_;
    const std::array<Buffer*,14>& src_gpu_;
    const std::array<uint32_t,14>& src_base_;
    std::string shader_dir_;
    std::string sidecar_;
    Buffer exec_{};
    bool resident_=false;
    bool validated_=false;
    BackendCounters counters_{};
    ImageValidation last_validation_{};
    std::string last_error_;
};

} // namespace
