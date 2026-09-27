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

#define main arcllm_q4_backend_p8c_main_disabled
#include "p8c_segmented_access_correctness.cpp"
#undef main

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
struct ComponentAgg {
    double max_abs=0.0;
    long double sq=0.0L;
    uint64_t n=0;
    bool finite=true;
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

static std::string target_name(uint32_t l){
    return std::string("blk.")+std::to_string(l)+".ffn_down.weight";
}
static std::string file_sha256(const std::string& path){
    std::ifstream f(path,std::ios::binary);
    if(!f)throw std::runtime_error("cannot open for SHA256: "+path);
    Sha256 h;
    std::vector<uint8_t> b(4u*1024u*1024u,0u);
    for(;;){
        f.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size()));
        const std::streamsize n=f.gcount();
        if(n>0)h.update(b.data(),size_t(n));
        if(f.eof())break;
        if(!f)throw std::runtime_error("SHA256 read failed: "+path);
    }
    return Sha256::hex(h.final());
}
static std::string raw_sha256(const void* p,uint64_t n){
    Sha256 h;
    h.update(reinterpret_cast<const uint8_t*>(p),size_t(n));
    return Sha256::hex(h.final());
}
static double rmse(const ComponentAgg& g){
    return g.n?std::sqrt(double(g.sq/static_cast<long double>(g.n)))
              :std::numeric_limits<double>::infinity();
}
static bool component_pass(const ComponentAgg& g){
    return g.finite&&g.n==50176ull&&g.max_abs<=0.02&&rmse(g)<=0.005;
}
static void accum(ComponentAgg& g,const float* ref,const float* got,uint32_t n){
    for(uint32_t i=0;i<n;++i){
        const double a=double(ref[i]),b=double(got[i]),d=std::abs(a-b);
        g.finite=g.finite&&std::isfinite(a)&&std::isfinite(b);
        g.max_abs=(std::max)(g.max_abs,d);
        g.sq+=static_cast<long double>(d*d);
        ++g.n;
    }
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

static ComponentAgg component_oracle(
    VkRuntime& vk,
    const std::array<Buffer*,14>& src_buffers,
    const std::array<uint32_t,14>& src_base,
    const Buffer* exec,
    bool use_a,
    const std::string& shader_dir){
    std::vector<float> x(kFFN);
    for(uint32_t k=0;k<kFFN;++k)
        x[k]=float(((int64_t(k)*37ll+11ll)%257ll)-128ll)/128.0f;
    float zero=0.0f;
    Buffer bx=vk.make_buffer(uint64_t(kFFN)*sizeof(float),x.data());
    Buffer bd=vk.make_buffer(sizeof(float),&zero);
    Buffer bref=vk.make_buffer(uint64_t(kHidden)*sizeof(float),nullptr);
    Buffer bgot=vk.make_buffer(uint64_t(kHidden)*sizeof(float),nullptr);
    ComponentAgg agg;
    try{
        for(uint32_t slot=0;slot<14u;++slot){
            DispatchOp base{
                "Q4V4.REFERENCE",
                join_path_p8c(shader_dir,"p7_q4k_gemm_2d.spv"),
                {src_buffers[slot],&bx,&bd,&bref},
                push_bytes(Q4PCGemm{kFFN,kHidden,1u,kSourceRowBytes,0u,src_base[slot],0u}),
                56u,1u,1u
            };
            std::vector<DispatchOp> vb{base};
            auto cb=vk.prepare_chain(vb);
            (void)vk.execute_prepared(cb,vb,false);
            vk.destroy_prepared(cb);

            DispatchOp cand;
            if(use_a){
                cand={
                    "Q4V4.A_SPLIT_K32",
                    join_path_p8c(shader_dir,"sa1_q4k_subgroup_splitk.spv"),
                    {src_buffers[slot],&bx,&bd,&bgot},
                    push_bytes(Q4PCGemm{kFFN,kHidden,1u,kSourceRowBytes,0u,src_base[slot],0u}),
                    896u,1u,1u
                };
            }else{
                if(!exec||!exec->buffer)
                    throw std::runtime_error("B execution requested without resident EXEC148");
                cand={
                    "Q4V4.B_EXEC148",
                    join_path_p8c(shader_dir,"q4_down_exec148_serial.spv"),
                    {const_cast<Buffer*>(exec),&bx,&bd,&bgot},
                    push_bytes(Q4PCGemm{
                        kFFN,kHidden,1u,kExecRowBytes,0u,
                        uint32_t(uint64_t(slot)*kExecLayerBytes),0u}),
                    56u,1u,1u
                };
            }
            std::vector<DispatchOp> vc{cand};
            auto cc=vk.prepare_chain(vc);
            (void)vk.execute_prepared(cc,vc,false);
            vk.destroy_prepared(cc);
            accum(
                agg,
                reinterpret_cast<const float*>(bref.mapped),
                reinterpret_cast<const float*>(bgot.mapped),
                kHidden);
        }
    }catch(...){
        vk.destroy_buffer(bgot);
        vk.destroy_buffer(bref);
        vk.destroy_buffer(bd);
        vk.destroy_buffer(bx);
        throw;
    }
    vk.destroy_buffer(bgot);
    vk.destroy_buffer(bref);
    vk.destroy_buffer(bd);
    vk.destroy_buffer(bx);
    return agg;
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

    ComponentAgg execute_component(bind::PrimitiveHandle primitive){
        if(primitive.opaque==q4reg::kPrimitiveA.value)
            return component_oracle(vk_,src_gpu_,src_base_,nullptr,true,shader_dir_);
        if(primitive.opaque==q4reg::kPrimitiveB.value&&resident_&&validated_)
            return component_oracle(vk_,src_gpu_,src_base_,&exec_,false,shader_dir_);
        throw std::runtime_error("unresolved primitive execution");
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

static void require(bool v,const char* msg){
    if(!v)throw std::runtime_error(msg);
}

static gp::PolicyDecision decide(
    const reg::PrimitiveRegistry& registry,
    const Q4VulkanBackendV4& backend,
    uint64_t h,
    bool p1_available,
    bool p3_available,
    bool p0_available){
    gp::PrimitiveRuntimeState ps[]={
        {q4reg::kPrimitiveA,false,true,true,true,true},
        {q4reg::kPrimitiveB,backend.resident(),backend.validated(),true,
         backend.resident()&&backend.validated(),true}
    };
    gp::AcquisitionRuntimeState as[]={
        {q4reg::kAcquirePrimary,p1_available,false},
        {q4reg::kAcquireSecondary,p3_available,false},
        {q4reg::kAcquireTertiary,p0_available,false},
        {q4reg::kAcquireDisabledWarm,false,false}
    };
    gp::PolicyRequest r{};
    r.capability=q4reg::kCapability;
    r.profile=q4reg::kProfile0;
    r.request_within_capability_evidence_scope=true;
    r.model_loaded=true;
    r.future_reuse_known=true;
    r.future_reuse_units=h;
    r.acquisition_allowed=true;
    r.primitive_states=ps;
    r.primitive_state_count=2;
    r.acquisition_states=as;
    r.acquisition_state_count=4;
    return gp::evaluate(registry,r);
}

static void write_component(std::ostream& o,const ComponentAgg& c){
    o<<"{\"finite\":"<<(c.finite?"true":"false")
     <<",\"n\":"<<c.n
     <<",\"max_abs\":"<<std::setprecision(15)<<c.max_abs
     <<",\"rmse\":"<<rmse(c)
     <<",\"pass\":"<<(component_pass(c)?"true":"false")<<"}";
}

} // namespace

#ifndef ARCLLM_Q4_VULKAN_BACKEND_V4_LIBRARY_ONLY
int main(int argc,char** argv){
    std::string model,shader_dir,sidecar,out="Q4_VULKAN_BACKEND_V4_RESULT.json";
    try{
        for(int i=1;i<argc;++i){
            const std::string a=argv[i];
            auto need=[&](const char* flag){
                if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+flag);
                return std::string(argv[++i]);
            };
            if(a=="--model")model=need("--model");
            else if(a=="--shader-dir")shader_dir=need("--shader-dir");
            else if(a=="--sidecar")sidecar=need("--sidecar");
            else if(a=="--out")out=need("--out");
        }
        if(model.empty()||shader_dir.empty())
            throw std::runtime_error("--model and --shader-dir are required");
        require(file_sha256(model)==kModelSha,"exact model SHA256 mismatch");

        GgufInfo gguf=GgufReader(model).read();
        TensorStore store;
        const TensorStoreReport ts=store.inspect(model,gguf);
        require(ts.mapped&&ts.all_bounds_valid&&ts.no_overlap&&
                ts.supported_types_only&&ts.q4_k_direct_access,
                "TensorStore invariants failed");
        const uint8_t* payload=store.mapped_base()+gguf.data_offset;

        std::array<const uint8_t*,14> src_cpu{};
        std::array<const GgufTensorInfo*,14> targets{};
        for(uint32_t slot=0;slot<14u;++slot){
            const auto* t=find_tensor(gguf,target_name(kQ4Layers[slot]));
            require(t&&t->ggml_type==12u&&t->dims.size()>=2u&&
                    t->dims[0]==kFFN&&t->dims[1]==kHidden,
                    "Q4 target tensor mismatch");
            require(tensor_nbytes(*t)==kSourceLayerBytes,"Q4 target bytes mismatch");
            src_cpu[slot]=payload+t->offset;
            targets[slot]=t;
        }

        VkRuntime vk;
        vk.init();
        std::vector<Buffer> arenas;
        arenas.reserve(kFrozenWeightArenas.size());
        uint64_t arena_bytes=0;
        for(const auto& a:kFrozenWeightArenas){
            const uint64_t bytes=a.end-a.start;
            arena_bytes+=bytes;
            arenas.push_back(vk.make_buffer(bytes,payload+a.start));
        }
        require(arena_bytes==4677120000ull,"frozen arena bytes mismatch");

        std::array<Buffer*,14> src_gpu{};
        std::array<uint32_t,14> src_base{};
        for(uint32_t slot=0;slot<14u;++slot){
            const uint64_t s=targets[slot]->offset,e=s+kSourceLayerBytes;
            bool found=false;
            for(uint32_t ai=0;ai<uint32_t(kFrozenWeightArenas.size());++ai){
                const auto& a=kFrozenWeightArenas[ai];
                if(s>=a.start&&e<=a.end){
                    const uint64_t base=s-a.start;
                    require(base<=0xffffffffull,"source base exceeds uint32");
                    src_gpu[slot]=&arenas[ai];
                    src_base[slot]=uint32_t(base);
                    found=true;
                    break;
                }
            }
            require(found,"Q4 target not in frozen arena");
        }

        Q4VulkanBackendV4 backend(
            vk,src_cpu,src_gpu,src_base,shader_dir,sidecar);
        reg::PrimitiveRegistry registry;
        reg::RegistryError rerr{};
        require(
            registry.add_bundle(q4reg::q4k_down_reference_bundle(),&rerr)==reg::RegistryStatus::OK,
            "Q4 registration failed");
        bind::BindingState binding_state{};

        // S1: H=1 must route A with no B allocation/materialization.
        const auto d1=decide(registry,backend,1u,true,backend.p3_available(),true);
        require(d1.status==gp::PolicyStatus::OK&&
                d1.route==q4reg::kPrimitiveA&&
                d1.lifecycle==gp::LifecycleAction::NONE,
                "S1 policy did not route A/NONE");
        const auto a1=bind::apply_decision(
            registry,q4reg::kCapability,d1,backend,binding_state);
        require(a1.ready&&a1.primitive.opaque==q4reg::kPrimitiveA.value,
                "S1 A resolve failed");
        require(backend.counters().b_allocations==0u&&
                backend.counters().b_materializations==0u&&
                !backend.resident(),
                "S1 hidden B acquisition detected");
        const ComponentAgg ca1=backend.execute_component(a1.primitive);
        require(component_pass(ca1),"S1 A component correctness failed");

        // S2: make P1 unavailable at H=2. Policy must stay on A; adapter must not retry.
        const auto d2=decide(registry,backend,2u,false,false,true);
        require(d2.status==gp::PolicyStatus::OK&&
                d2.route==q4reg::kPrimitiveA&&
                d2.lifecycle==gp::LifecycleAction::NONE,
                "S2 policy hidden acquisition selection");
        const auto a2=bind::apply_decision(
            registry,q4reg::kCapability,d2,backend,binding_state);
        require(a2.ready&&a2.primitive.opaque==q4reg::kPrimitiveA.value,
                "S2 A resolve failed");
        require(backend.counters().b_allocations==0u&&
                backend.counters().p3_calls==0u&&backend.counters().p0_calls==0u,
                "S2 adapter hidden retry/acquisition detected");

        // S3: H=2 with P1 available must explicitly acquire, validate, then route B.
        const auto d3=decide(registry,backend,2u,true,backend.p3_available(),true);
        require(d3.status==gp::PolicyStatus::OK&&
                d3.route==q4reg::kPrimitiveB&&
                d3.lifecycle==gp::LifecycleAction::ACQUIRE&&
                d3.acquisition==q4reg::kAcquirePrimary,
                "S3 policy did not select P1 ACQUIRE");
        const auto a3=bind::apply_decision(
            registry,q4reg::kCapability,d3,backend,binding_state);
        require(a3.ready&&a3.primitive.opaque==q4reg::kPrimitiveB.value,
                "S3 B resolve failed");
        require(backend.resident()&&backend.validated()&&
                backend.counters().b_allocations==1u&&
                backend.counters().b_materializations==1u&&
                backend.counters().p1_calls==1u&&
                backend.counters().b_validations==1u,
                "S3 acquisition accounting mismatch");
        const auto v3=backend.last_validation();
        require(v3.exact&&v3.raw_sha==kCanonicalRawSha,
                "S3 exact B identity mismatch");
        const ComponentAgg cb1=backend.execute_component(a3.primitive);
        require(component_pass(cb1),"S3 B component correctness failed");

        // S4: once B is resident, positive H reuses B with no reacquisition.
        const auto before_reuse=backend.counters();
        const auto d4=decide(registry,backend,1u,true,backend.p3_available(),true);
        require(d4.status==gp::PolicyStatus::OK&&
                d4.route==q4reg::kPrimitiveB&&
                d4.lifecycle==gp::LifecycleAction::NONE,
                "S4 resident B hysteresis failed");
        const auto a4=bind::apply_decision(
            registry,q4reg::kCapability,d4,backend,binding_state);
        require(a4.ready&&a4.primitive.opaque==q4reg::kPrimitiveB.value,
                "S4 B reuse resolve failed");
        require(backend.counters().b_allocations==before_reuse.b_allocations&&
                backend.counters().b_materializations==before_reuse.b_materializations,
                "S4 duplicate B acquisition detected");

        // S5: known H=0 must evict B then route A.
        const auto d5=decide(registry,backend,0u,true,backend.p3_available(),true);
        require(d5.status==gp::PolicyStatus::OK&&
                d5.route==q4reg::kPrimitiveA&&
                d5.lifecycle==gp::LifecycleAction::EVICT,
                "S5 policy did not evict B");
        const auto a5=bind::apply_decision(
            registry,q4reg::kCapability,d5,backend,binding_state);
        require(a5.ready&&a5.primitive.opaque==q4reg::kPrimitiveA.value,
                "S5 A resolve after eviction failed");
        require(!backend.resident()&&
                backend.counters().b_releases==1u,
                "S5 B release mismatch");

        // S6: tertiary P0 is selected by policy, never by hidden backend retry.
        const auto d6=decide(registry,backend,17u,false,false,true);
        require(d6.status==gp::PolicyStatus::OK&&
                d6.route==q4reg::kPrimitiveB&&
                d6.lifecycle==gp::LifecycleAction::ACQUIRE&&
                d6.acquisition==q4reg::kAcquireTertiary,
                "S6 policy did not select P0 ACQUIRE");
        const auto a6=bind::apply_decision(
            registry,q4reg::kCapability,d6,backend,binding_state);
        require(a6.ready&&a6.primitive.opaque==q4reg::kPrimitiveB.value,
                "S6 P0 B resolve failed");
        require(backend.counters().p0_calls==1u&&
                backend.counters().b_allocations==2u&&
                backend.counters().b_materializations==2u&&
                backend.counters().b_validations==2u,
                "S6 P0 acquisition accounting mismatch");
        const ComponentAgg cb2=backend.execute_component(a6.primitive);
        require(component_pass(cb2),"S6 P0 B component correctness failed");

        const auto d7=decide(registry,backend,0u,false,false,true);
        const auto a7=bind::apply_decision(
            registry,q4reg::kCapability,d7,backend,binding_state);
        require(a7.ready&&a7.primitive.opaque==q4reg::kPrimitiveA.value&&
                !backend.resident()&&backend.counters().b_releases==2u,
                "S7 final eviction failed");

        // S8/S9: when a canonical sidecar is supplied, P3-cold must be selected
        // by policy and executed exactly once, then explicitly evicted.
        bool p3_executed=false;
        ComponentAgg cb3{};
        if(backend.p3_available()){
            const auto d8=decide(registry,backend,15u,false,true,true);
            require(d8.status==gp::PolicyStatus::OK&&
                    d8.route==q4reg::kPrimitiveB&&
                    d8.lifecycle==gp::LifecycleAction::ACQUIRE&&
                    d8.acquisition==q4reg::kAcquireSecondary,
                    "S8 policy did not select P3-cold ACQUIRE");
            const auto a8=bind::apply_decision(
                registry,q4reg::kCapability,d8,backend,binding_state);
            require(a8.ready&&a8.primitive.opaque==q4reg::kPrimitiveB.value,
                    "S8 P3 B resolve failed");
            require(backend.counters().p3_calls==1u&&
                    backend.counters().b_allocations==3u&&
                    backend.counters().b_materializations==3u&&
                    backend.counters().b_validations==3u,
                    "S8 P3 acquisition accounting mismatch");
            cb3=backend.execute_component(a8.primitive);
            require(component_pass(cb3),"S8 P3 B component correctness failed");
            const auto d9=decide(registry,backend,0u,false,true,true);
            const auto a9=bind::apply_decision(
                registry,q4reg::kCapability,d9,backend,binding_state);
            require(a9.ready&&a9.primitive.opaque==q4reg::kPrimitiveA.value&&
                    !backend.resident()&&backend.counters().b_releases==3u,
                    "S9 P3 final eviction failed");
            p3_executed=true;
        }

        const auto c=backend.counters();
        std::ofstream o(out,std::ios::binary|std::ios::trunc);
        require(bool(o),"cannot write backend result");
        o<<"{\n";
        o<<"\"schema\":\"arcllm.v1.phase2.q4_vulkan_backend_v4.validation.v0.1\",\n";
        o<<"\"status\":\"PASS_Q4_VULKAN_BACKEND_V4_CONVERGENCE_QA\",\n";
        o<<"\"model_sha256\":\""<<kModelSha<<"\",\n";
        o<<"\"surface\":{\"policy\":\"v4\",\"binding\":\"v4\",\"backend\":\"Q4_VULKAN_BACKEND_V4\"},\n";
        o<<"\"scenarios\":{\"A_no_acquire\":true,\"P1_unavailable_no_hidden_retry\":true,"
           <<"\"P1_explicit_acquire\":true,\"resident_B_reuse\":true,"
           <<"\"explicit_evict\":true,\"P0_policy_selected_acquire\":true,"
           <<"\"P3_cold_explicit_acquire\":"<<(p3_executed?"true":"false")<<"},\n";
        o<<"\"counters\":{\"b_allocations\":"<<c.b_allocations
         <<",\"b_materializations\":"<<c.b_materializations
         <<",\"b_validations\":"<<c.b_validations
         <<",\"b_releases\":"<<c.b_releases
         <<",\"p1_calls\":"<<c.p1_calls
         <<",\"p3_calls\":"<<c.p3_calls
         <<",\"p0_calls\":"<<c.p0_calls
         <<",\"resolve_a\":"<<c.resolve_a
         <<",\"resolve_b\":"<<c.resolve_b<<"},\n";
        o<<"\"A_component\":";write_component(o,ca1);o<<",\n";
        o<<"\"B_P1_component\":";write_component(o,cb1);o<<",\n";
        o<<"\"B_P0_component\":";write_component(o,cb2);o<<",\n";
        o<<"\"B_P3_component\":";if(p3_executed)write_component(o,cb3);else o<<"null";o<<",\n";
        o<<"\"exec148_identity\":{\"tuple_sha256\":\""<<v3.exec_tuple_sha
         <<"\",\"raw_sha256\":\""<<v3.raw_sha
         <<"\",\"bytes\":"<<kExecFamilyBytes<<"},\n";
        o<<"\"performance\":{\"authorized\":false,\"timing_emitted\":false,\"counters_executed\":false},\n";
        o<<"\"hidden_retry\":false,\n";
        o<<"\"hidden_eager_B_acquisition\":false\n";
        o<<"}\n";
        o.close();

        for(auto& b:arenas)vk.destroy_buffer(b);
        std::cout<<"Q4_VULKAN_BACKEND_V4_CONVERGENCE_QA=PASS\n";
        std::cout<<"A_WITHOUT_B_ALLOCATION=PASS\n";
        std::cout<<"P1_EXPLICIT_ACQUIRE_VALIDATE_ROUTE_B=PASS\n";
        std::cout<<"RESIDENT_B_REUSE_NO_REACQUIRE=PASS\n";
        std::cout<<"EVICT_B_THEN_A=PASS\n";
        std::cout<<"P0_EXPLICIT_POLICY_SELECTED_ACQUIRE=PASS\n";
        std::cout<<"P3_COLD_EXPLICIT_POLICY_SELECTED_ACQUIRE="<<(p3_executed?"PASS":"NOT_RUN_NO_SIDECAR")<<"\n";
        std::cout<<"NO_TIMING_NO_COUNTERS\n";
        return 0;
    }catch(const std::exception& e){
        std::ofstream o(out,std::ios::binary|std::ios::trunc);
        if(o)o<<"{\"schema\":\"arcllm.v1.phase2.q4_vulkan_backend_v4.validation.v0.1\","
              <<"\"status\":\"ERROR\",\"error\":\"backend convergence execution failed\"}\n";
        std::cerr<<"Q4_VULKAN_BACKEND_V4 STOP: "<<e.what()<<"\n";
        return 2;
    }
}
#endif // ARCLLM_Q4_VULKAN_BACKEND_V4_LIBRARY_ONLY
