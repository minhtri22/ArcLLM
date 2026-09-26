#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "gguf.h"
#include "tensor_store.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define main arcllm_b1_2_runtime_main_disabled
#include "arcllm_v1_q4_down_4arm_timing_runtime.cpp"
#undef main

#include "q4_down_exec148_materializer.h"

namespace {
using namespace arcllm_exec148;

static constexpr std::array<uint32_t,14> kLayers={3u,4u,6u,7u,8u,11u,12u,14u,15u,17u,18u,19u,21u,22u};
static constexpr uint32_t kFFN=18944u;
static constexpr uint32_t kHidden=3584u;
static constexpr uint32_t kBlocksPerLayer=kRows*kBlocksPerRow;
static constexpr uint32_t kP1LocalSize=256u;
static constexpr uint32_t kP1Groups=kBlocksPerLayer/kP1LocalSize;
static constexpr uint64_t kP3ChunkBytes=4ull*1024ull*1024ull;
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
static constexpr char kCanonicalFamilyHash[]="60565f9f0b12de4884e884d8311263df7238679745c83393a695935cd3eccbb2";
static constexpr char kModelSha256[]="60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463";

struct PCMat {
    uint32_t src_base_bytes;
    uint32_t dst_base_bytes;
    uint32_t block_count;
    uint32_t reserved;
};
struct PCGemm {
    uint32_t n,rows,batch,row_bytes,add_bias,w_base_bytes,bias_base;
};
struct ImageValidation {
    bool exact=false;
    uint64_t blocks=0;
    std::string source_family_hash;
    std::string exec_family_hash;
};
struct ComponentAgg {
    double max_abs=0.0;
    long double sq=0.0L;
    uint64_t n=0;
    bool finite=true;
};

static std::string jesc(const std::string&s){
    std::string o;
    for(char c:s){
        if(c=='\\'||c=='"'){o.push_back('\\');o.push_back(c);}
        else if(c=='\n')o+="\\n";
        else if(c=='\r')o+="\\r";
        else if(c=='\t')o+="\\t";
        else o.push_back(c);
    }
    return o;
}

static std::string target_name(uint32_t l){
    return std::string("blk.")+std::to_string(l)+".ffn_down.weight";
}

static std::string raw_sha256(const void*p,uint64_t n){
    Sha256 h;
    h.update(reinterpret_cast<const uint8_t*>(p),size_t(n));
    return Sha256::hex(h.final());
}

static ImageValidation validate_image(const std::array<const uint8_t*,14>&src,const uint8_t*exec){
    Sha256 hs,he;
    ImageValidation v;
    v.exact=true;
    for(uint32_t slot=0;slot<14u;++slot){
        auto r=validate_tensor(src[slot],exec+uint64_t(slot)*kExecLayerBytes,kRows,kSourceRowBytes,kExecRowBytes,&hs,&he);
        v.exact=v.exact&&r.exact&&r.blocks_checked==uint64_t(kBlocksPerLayer);
        v.blocks+=r.blocks_checked;
    }
    v.source_family_hash=Sha256::hex(hs.final());
    v.exec_family_hash=Sha256::hex(he.final());
    v.exact=v.exact&&v.source_family_hash==kCanonicalFamilyHash&&v.exec_family_hash==kCanonicalFamilyHash;
    return v;
}

static double rmse(const ComponentAgg&g){
    return g.n?std::sqrt(double(g.sq/static_cast<long double>(g.n))):std::numeric_limits<double>::infinity();
}
static bool component_pass(const ComponentAgg&g){
    return g.finite&&g.n==50176ull&&g.max_abs<=0.02&&rmse(g)<=0.005;
}
static void accum(ComponentAgg&g,const float*ref,const float*got,uint32_t n){
    for(uint32_t i=0;i<n;++i){
        double a=double(ref[i]),b=double(got[i]),d=std::abs(a-b);
        g.finite=g.finite&&std::isfinite(a)&&std::isfinite(b);
        g.max_abs=(std::max)(g.max_abs,d);
        g.sq+=static_cast<long double>(d*d);
        ++g.n;
    }
}

static ComponentAgg component_oracle(
    VkRuntime&vk,
    const std::array<Buffer*,14>&src_buffers,
    const std::array<uint32_t,14>&src_base_bytes,
    Buffer&exec,
    const std::string&shader_dir){

    std::vector<float>x(kFFN);
    for(uint32_t k=0;k<kFFN;++k)x[k]=float(((int64_t(k)*37ll+11ll)%257ll)-128ll)/128.0f;
    float zero=0.0f;
    Buffer bx=vk.make_buffer(uint64_t(kFFN)*sizeof(float),x.data());
    Buffer bd=vk.make_buffer(sizeof(float),&zero);
    Buffer bref=vk.make_buffer(uint64_t(kHidden)*sizeof(float),nullptr);
    Buffer bgot=vk.make_buffer(uint64_t(kHidden)*sizeof(float),nullptr);
    ComponentAgg agg;

    for(uint32_t slot=0;slot<14u;++slot){
        DispatchOp op0{
            "B1.2.P0_COMPONENT",
            join_path_p8c(shader_dir,"p7_q4k_gemm_2d.spv"),
            {src_buffers[slot],&bx,&bd,&bref},
            push_bytes(PCGemm{kFFN,kHidden,1u,kSourceRowBytes,0u,src_base_bytes[slot],0u}),
            56u,1u,1u
        };
        std::vector<DispatchOp>v0{op0};
        auto c0=vk.prepare_chain(v0);
        (void)vk.execute_prepared(c0,v0,false);
        vk.destroy_prepared(c0);

        DispatchOp opb{
            "B1.2.B_COMPONENT",
            join_path_p8c(shader_dir,"q4_down_exec148_serial.spv"),
            {&exec,&bx,&bd,&bgot},
            push_bytes(PCGemm{kFFN,kHidden,1u,kExecRowBytes,0u,uint32_t(uint64_t(slot)*kExecLayerBytes),0u}),
            56u,1u,1u
        };
        std::vector<DispatchOp>vb{opb};
        auto cb=vk.prepare_chain(vb);
        (void)vk.execute_prepared(cb,vb,false);
        vk.destroy_prepared(cb);

        accum(agg,reinterpret_cast<const float*>(bref.mapped),reinterpret_cast<const float*>(bgot.mapped),kHidden);
    }

    vk.destroy_buffer(bgot);
    vk.destroy_buffer(bref);
    vk.destroy_buffer(bd);
    vk.destroy_buffer(bx);
    return agg;
}

static std::vector<DispatchOp> build_p1_ops(
    const std::array<Buffer*,14>&src_buffers,
    const std::array<uint32_t,14>&src_base_bytes,
    Buffer&exec,
    const std::string&shader_dir){

    static_assert(kBlocksPerLayer%kP1LocalSize==0u,"P1 block geometry must divide exactly");
    std::vector<DispatchOp>ops;
    ops.reserve(14u);
    for(uint32_t slot=0;slot<14u;++slot){
        PCMat pc{src_base_bytes[slot],uint32_t(uint64_t(slot)*kExecLayerBytes),kBlocksPerLayer,0u};
        ops.push_back({
            std::string("B1.2.P1.L")+std::to_string(kLayers[slot]),
            join_path_p8c(shader_dir,"b1_2_exec148_gpu_materialize.comp.spv"),
            {src_buffers[slot],&exec},
            push_bytes(pc),
            kP1Groups,1u,1u
        });
    }
    return ops;
}

static std::string write_sidecar(
    const std::array<const uint8_t*,14>&src,
    const std::string&sidecar){

    std::filesystem::path p(sidecar);
    if(p.has_parent_path())std::filesystem::create_directories(p.parent_path());
    std::ofstream f(sidecar,std::ios::binary|std::ios::trunc);
    if(!f)throw std::runtime_error("cannot create P3 sidecar");
    std::vector<uint8_t>layer(size_t(kExecLayerBytes));
    Sha256 raw;
    for(uint32_t slot=0;slot<14u;++slot){
        materialize_tensor(src[slot],layer.data());
        raw.update(layer.data(),layer.size());
        f.write(reinterpret_cast<const char*>(layer.data()),std::streamsize(layer.size()));
        if(!f)throw std::runtime_error("P3 sidecar write failed");
    }
    f.close();
    if(std::filesystem::file_size(p)!=kExecFamilyBytes)throw std::runtime_error("P3 sidecar byte count mismatch");
    std::string h=Sha256::hex(raw.final());

    std::ofstream m(sidecar+".manifest.json",std::ios::binary|std::ios::trunc);
    if(!m)throw std::runtime_error("cannot create P3 sidecar manifest");
    m<<"{\n";
    m<<"  \"schema\":\"arcllm.v1.b1_2.exec148_sidecar.v0.1\",\n";
    m<<"  \"source_model_sha256\":\""<<kModelSha256<<"\",\n";
    m<<"  \"exec148_id\":\"Q4K_SERIAL_K_EXEC148_V0_1\",\n";
    m<<"  \"payload_bytes\":"<<kExecFamilyBytes<<",\n";
    m<<"  \"canonical_family_hash\":\""<<kCanonicalFamilyHash<<"\",\n";
    m<<"  \"payload_raw_sha256\":\""<<h<<"\",\n";
    m<<"  \"layers\":[3,4,6,7,8,11,12,14,15,17,18,19,21,22],\n";
    m<<"  \"shape\":{\"k\":18944,\"rows\":3584,\"batch\":1}\n";
    m<<"}\n";
    return h;
}

static void buffered_preload(const std::string&sidecar){
    std::ifstream f(sidecar,std::ios::binary);
    if(!f)throw std::runtime_error("cannot open sidecar for warm preload");
    std::vector<char>buf(size_t(kP3ChunkBytes));
    uint64_t total=0;
    while(total<kExecFamilyBytes){
        uint64_t want=(std::min)(kP3ChunkBytes,kExecFamilyBytes-total);
        f.read(buf.data(),std::streamsize(want));
        if(uint64_t(f.gcount())!=want)throw std::runtime_error("warm preload short read");
        total+=want;
    }
}

static void buffered_load(const std::string&sidecar,uint8_t*dst){
    std::ifstream f(sidecar,std::ios::binary);
    if(!f)throw std::runtime_error("cannot open sidecar for warm load");
    uint64_t off=0;
    while(off<kExecFamilyBytes){
        uint64_t want=(std::min)(kP3ChunkBytes,kExecFamilyBytes-off);
        f.read(reinterpret_cast<char*>(dst+off),std::streamsize(want));
        if(uint64_t(f.gcount())!=want)throw std::runtime_error("warm load short read");
        off+=want;
    }
}

static uint32_t unbuffered_sector_bytes(const std::string&sidecar){
    std::wstring abs=std::filesystem::absolute(std::filesystem::path(sidecar)).wstring();
    wchar_t root[MAX_PATH]{};
    if(!GetVolumePathNameW(abs.c_str(),root,MAX_PATH))throw std::runtime_error("GetVolumePathNameW failed");
    DWORD spc=0,bps=0,freec=0,totalc=0;
    if(!GetDiskFreeSpaceW(root,&spc,&bps,&freec,&totalc)||bps==0u)throw std::runtime_error("GetDiskFreeSpaceW failed");
    if((4096u%bps)!=0u||(kP3ChunkBytes%bps)!=0u||(kExecFamilyBytes%bps)!=0u)
        throw std::runtime_error("P3 frozen 4KiB/4MiB unbuffered alignment incompatible with volume sector");
    return bps;
}

static void unbuffered_load(const std::string&sidecar,uint8_t*dst,uint32_t&sector_bytes){
    sector_bytes=unbuffered_sector_bytes(sidecar);
    std::wstring path=std::filesystem::absolute(std::filesystem::path(sidecar)).wstring();
    HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("CreateFileW P3 cold-unbuffered failed");
    void*staging=VirtualAlloc(nullptr,size_t(kP3ChunkBytes),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!staging){CloseHandle(h);throw std::runtime_error("VirtualAlloc P3 staging failed");}
    try{
        uint64_t off=0;
        while(off<kExecFamilyBytes){
            DWORD want=DWORD((std::min)(kP3ChunkBytes,kExecFamilyBytes-off));
            if((want%sector_bytes)!=0u)throw std::runtime_error("P3 cold read is not sector aligned");
            DWORD got=0;
            if(!ReadFile(h,staging,want,&got,nullptr)||got!=want)throw std::runtime_error("P3 cold-unbuffered short read");
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

static void write_common_result(
    std::ostream&o,
    const char*mode,
    const ImageValidation&v,
    const ComponentAgg&c,
    const std::string&raw_hash){

    o<<"\"mode\":\""<<mode<<"\",\n";
    o<<"\"performance\":{\"authorized\":false,\"acquisition_timing_emitted\":false,\"gpu_timing_emitted\":false,\"storage_timing_emitted\":false},\n";
    o<<"\"exec148\":{\"bytes\":"<<kExecFamilyBytes
     <<",\"canonical_expected\":\""<<kCanonicalFamilyHash
     <<"\",\"source_family_sha256\":\""<<v.source_family_hash
     <<"\",\"exec_family_sha256\":\""<<v.exec_family_hash
     <<"\",\"tuple_exact\":"<<(v.exact?"true":"false")
     <<",\"blocks_checked\":"<<v.blocks
     <<",\"raw_sha256\":\""<<raw_hash<<"\"},\n";
    o<<"\"component\":{\"max_abs\":"<<std::setprecision(15)<<c.max_abs
     <<",\"rmse\":"<<rmse(c)<<",\"n\":"<<c.n
     <<",\"finite\":"<<(c.finite?"true":"false")
     <<",\"pass\":"<<(component_pass(c)?"true":"false")<<"}";
}

}

int main(int argc,char**argv){
    std::string model,shader_dir,mode,sidecar,out="b1_2_zero_science.json";
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            auto need=[&](const char*f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};
            if(a=="--model")model=need("--model");
            else if(a=="--shader-dir")shader_dir=need("--shader-dir");
            else if(a=="--mode")mode=need("--mode");
            else if(a=="--sidecar")sidecar=need("--sidecar");
            else if(a=="--out")out=need("--out");
        }
        if(model.empty()||shader_dir.empty()||(mode!="P1"&&mode!="P3"))throw std::runtime_error("B1.2 required arguments missing");
        if(mode=="P3"&&sidecar.empty())throw std::runtime_error("P3 requires --sidecar");

        GgufInfo gguf=GgufReader(model).read();
        TensorStore store;
        TensorStoreReport ts=store.inspect(model,gguf);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("B1.2 TensorStore invariants failed");
        const uint8_t*payload=store.mapped_base()+gguf.data_offset;

        std::array<const uint8_t*,14>src_cpu{};
        std::array<const GgufTensorInfo*,14>targets{};
        for(uint32_t slot=0;slot<14u;++slot){
            const auto*t=find_tensor(gguf,target_name(kLayers[slot]));
            if(!t||t->ggml_type!=12u||t->dims.size()<2u||t->dims[0]!=kFFN||t->dims[1]!=kHidden)
                throw std::runtime_error("B1.2 target tensor mismatch: "+target_name(kLayers[slot]));
            if(tensor_nbytes(*t)!=kSourceLayerBytes)throw std::runtime_error("B1.2 target tensor bytes mismatch");
            src_cpu[slot]=payload+t->offset;
            targets[slot]=t;
        }

        VkRuntime vk;
        vk.init();
        std::vector<Buffer>arenas;
        arenas.reserve(kFrozenWeightArenas.size());
        uint64_t arena_bytes=0;
        for(const auto&a:kFrozenWeightArenas){
            uint64_t bytes=a.end-a.start;
            arena_bytes+=bytes;
            arenas.push_back(vk.make_buffer(bytes,payload+a.start));
        }
        if(arena_bytes!=4677120000ull)throw std::runtime_error("B1.2 frozen arena byte total mismatch");
        std::array<Buffer*,14>src_gpu{};
        std::array<uint32_t,14>src_base{};
        for(uint32_t slot=0;slot<14u;++slot){
            const uint64_t s=targets[slot]->offset,e=s+kSourceLayerBytes;
            bool found=false;
            for(uint32_t ai=0;ai<uint32_t(kFrozenWeightArenas.size());++ai){
                const auto&a=kFrozenWeightArenas[ai];
                if(s>=a.start&&e<=a.end){
                    uint64_t base=s-a.start;
                    if(base>0xffffffffull)throw std::runtime_error("B1.2 source base exceeds uint32");
                    src_gpu[slot]=&arenas[ai];
                    src_base[slot]=uint32_t(base);
                    found=true;
                    break;
                }
            }
            if(!found)throw std::runtime_error("B1.2 target tensor not contained in one frozen arena");
        }
        Buffer exec=vk.make_buffer(kExecFamilyBytes,nullptr);
        const VkMemoryPropertyFlags required_mem=
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if((vk.memory_type_flags()&required_mem)!=required_mem)throw std::runtime_error("B1.2 frozen UMA memory flags missing");

        if(mode=="P1"){
            auto ops=build_p1_ops(src_gpu,src_base,exec,shader_dir);
            if(ops.size()!=14u)throw std::runtime_error("P1 dispatch count mismatch");
            auto chain=vk.prepare_chain(ops);
            (void)vk.execute_prepared(chain,ops,false);
            vk.destroy_prepared(chain);

            ImageValidation v=validate_image(src_cpu,reinterpret_cast<const uint8_t*>(exec.mapped));
            ComponentAgg c=component_oracle(vk,src_gpu,src_base,exec,shader_dir);
            std::string raw=raw_sha256(exec.mapped,kExecFamilyBytes);
            bool pass=v.exact&&component_pass(c);

            std::ofstream o(out,std::ios::binary|std::ios::trunc);
            if(!o)throw std::runtime_error("cannot write P1 result");
            o<<"{\n\"schema\":\"arcllm.v1.b1_2.p1.zero_science_qualification.v0.1\",\n";
            o<<"\"status\":\""<<(pass?"PASS_P1_ZERO_SCIENCE_CORRECTNESS":"FAIL_P1_ZERO_SCIENCE_CORRECTNESS")<<"\",\n";
            write_common_result(o,"P1",v,c,raw);o<<",\n";
            o<<"\"data_path\":{\"dispatches\":14,\"blocks_per_layer\":"<<kBlocksPerLayer
             <<",\"local_size\":"<<kP1LocalSize<<",\"workgroups_per_layer\":"<<kP1Groups
             <<",\"full_image_cpu_staging\":false,\"final_buffer_direct_write\":true}\n}\n";
            if(!pass)throw std::runtime_error("P1 zero-science correctness failed");
        }else{
            std::string sidecar_raw=write_sidecar(src_cpu,sidecar);

            buffered_preload(sidecar);
            std::memset(exec.mapped,0,size_t(exec.size));
            buffered_load(sidecar,reinterpret_cast<uint8_t*>(exec.mapped));
            std::string warm_raw=raw_sha256(exec.mapped,kExecFamilyBytes);
            ImageValidation warm_v=validate_image(src_cpu,reinterpret_cast<const uint8_t*>(exec.mapped));
            ComponentAgg warm_c=component_oracle(vk,src_gpu,src_base,exec,shader_dir);
            bool warm_pass=warm_raw==sidecar_raw&&warm_v.exact&&component_pass(warm_c);

            std::memset(exec.mapped,0,size_t(exec.size));
            uint32_t sector=0;
            unbuffered_load(sidecar,reinterpret_cast<uint8_t*>(exec.mapped),sector);
            std::string cold_raw=raw_sha256(exec.mapped,kExecFamilyBytes);
            ImageValidation cold_v=validate_image(src_cpu,reinterpret_cast<const uint8_t*>(exec.mapped));
            ComponentAgg cold_c=component_oracle(vk,src_gpu,src_base,exec,shader_dir);
            bool cold_pass=cold_raw==sidecar_raw&&cold_v.exact&&component_pass(cold_c);
            bool pass=warm_pass&&cold_pass;

            std::ofstream o(out,std::ios::binary|std::ios::trunc);
            if(!o)throw std::runtime_error("cannot write P3 result");
            o<<"{\n\"schema\":\"arcllm.v1.b1_2.p3.zero_science_qualification.v0.1\",\n";
            o<<"\"status\":\""<<(pass?"PASS_P3_ZERO_SCIENCE_CORRECTNESS":"FAIL_P3_ZERO_SCIENCE_CORRECTNESS")<<"\",\n";
            o<<"\"performance\":{\"authorized\":false,\"acquisition_timing_emitted\":false,\"storage_timing_emitted\":false},\n";
            o<<"\"sidecar\":{\"path\":\""<<jesc(sidecar)<<"\",\"manifest_path\":\""<<jesc(sidecar+".manifest.json")
             <<"\",\"payload_bytes\":"<<kExecFamilyBytes<<",\"payload_raw_sha256\":\""<<sidecar_raw
             <<"\",\"canonical_family_hash\":\""<<kCanonicalFamilyHash<<"\",\"compression\":false},\n";
            o<<"\"warm\":{\"raw_sha256\":\""<<warm_raw<<"\",\"tuple_exact\":"<<(warm_v.exact?"true":"false")
             <<",\"source_family_sha256\":\""<<warm_v.source_family_hash<<"\",\"exec_family_sha256\":\""<<warm_v.exec_family_hash<<"\""
             <<",\"component_max_abs\":"<<std::setprecision(15)<<warm_c.max_abs<<",\"component_rmse\":"<<rmse(warm_c)
             <<",\"component_n\":"<<warm_c.n<<",\"component_pass\":"<<(component_pass(warm_c)?"true":"false")
             <<",\"pass\":"<<(warm_pass?"true":"false")<<"},\n";
            o<<"\"cold_unbuffered\":{\"raw_sha256\":\""<<cold_raw<<"\",\"sector_bytes\":"<<sector
             <<",\"chunk_bytes\":"<<kP3ChunkBytes<<",\"staging\":\"VirtualAlloc\",\"tuple_exact\":"<<(cold_v.exact?"true":"false")
             <<",\"source_family_sha256\":\""<<cold_v.source_family_hash<<"\",\"exec_family_sha256\":\""<<cold_v.exec_family_hash<<"\""
             <<",\"component_max_abs\":"<<cold_c.max_abs<<",\"component_rmse\":"<<rmse(cold_c)
             <<",\"component_n\":"<<cold_c.n<<",\"component_pass\":"<<(component_pass(cold_c)?"true":"false")
             <<",\"pass\":"<<(cold_pass?"true":"false")<<"}\n}\n";
            if(!pass)throw std::runtime_error("P3 zero-science correctness failed");
        }

        for(auto&b:arenas)vk.destroy_buffer(b);
        vk.destroy_buffer(exec);
        std::cout<<"B1_2_ZERO_SCIENCE="<<mode<<"_PASS\n";
        std::cout<<"NO_ACQUISITION_TIMING. NO_PERFORMANCE_ADJUDICATION.\n";
        return 0;
    }catch(const std::exception&e){
        std::cerr<<"B1.2 ZERO-SCIENCE STOP: "<<e.what()<<"\n";
        return 2;
    }
}
