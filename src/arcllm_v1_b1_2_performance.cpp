#define main arcllm_b1_2_zero_science_main_disabled
#include "arcllm_v1_b1_2_zero_science.cpp"
#undef main

#include <algorithm>
#include <chrono>
#include <fstream>
#include <numeric>

namespace {

static constexpr uint32_t kB12Attempts=8u;
static constexpr double kP0ReferenceMs=231.6382;
static constexpr double kDeltaWS=14.1842444453716;
static constexpr double kDeltaWC=6.422135259876299;
static constexpr char kExpectedSidecarRawSha[]="3f168749256e8acbbbe61da06196ca0e51a249b3923938e5651a4da6f50ab43f";

struct PerfSummary {
    double min=0.0,max=0.0,median=0.0,mad=0.0;
};
struct PerfState {
    std::vector<double> samples;
    bool correctness=true;
};

static double median_b12(std::vector<double> v){
    if(v.empty())throw std::runtime_error("B1.2 median empty");
    std::sort(v.begin(),v.end());
    const size_t n=v.size();
    return (n&1u)?v[n/2]:(v[n/2-1]+v[n/2])*0.5;
}
static PerfSummary summarize_b12(const std::vector<double>&v){
    if(v.size()!=kB12Attempts)throw std::runtime_error("B1.2 sample count mismatch");
    PerfSummary s;
    s.min=*std::min_element(v.begin(),v.end());
    s.max=*std::max_element(v.begin(),v.end());
    s.median=median_b12(v);
    std::vector<double>d;d.reserve(v.size());
    for(double x:v)d.push_back(std::abs(x-s.median));
    s.mad=median_b12(d);
    return s;
}
static double hstar(double acquisition_ms,double delta){
    return acquisition_ms/delta;
}
static uint64_t strict_tokens(double h){
    return uint64_t(std::floor(h))+1ull;
}
static std::string file_sha256_stream(const std::string&path){
    std::ifstream f(path,std::ios::binary);
    if(!f)throw std::runtime_error("cannot open file for SHA256: "+path);
    Sha256 h;
    std::vector<uint8_t>buf(4u*1024u*1024u,0u);
    for(;;){
        f.read(reinterpret_cast<char*>(buf.data()),std::streamsize(buf.size()));
        std::streamsize got=f.gcount();
        if(got>0)h.update(buf.data(),size_t(got));
        if(f.eof())break;
        if(!f)throw std::runtime_error("SHA256 file read failed: "+path);
    }
    return Sha256::hex(h.final());
}
static bool validate_attempt(
    VkRuntime&vk,
    const std::array<const uint8_t*,14>&src_cpu,
    const std::array<Buffer*,14>&src_gpu,
    const std::array<uint32_t,14>&src_base,
    Buffer&exec,
    const std::string&shader_dir){

    ImageValidation v=validate_image(src_cpu,reinterpret_cast<const uint8_t*>(exec.mapped));
    if(!v.exact||v.exec_family_hash!=kCanonicalFamilyHash)return false;
    if(raw_sha256(exec.mapped,kExecFamilyBytes)!=kExpectedSidecarRawSha)return false;
    ComponentAgg c=component_oracle(vk,src_gpu,src_base,exec,shader_dir);
    return component_pass(c);
}
static void write_samples(std::ostream&o,const std::vector<double>&v){
    o<<"[";
    for(size_t i=0;i<v.size();++i){if(i)o<<",";o<<std::setprecision(15)<<v[i];}
    o<<"]";
}
static void write_summary(std::ostream&o,const PerfSummary&s){
    o<<"{\"min\":"<<std::setprecision(15)<<s.min
     <<",\"max\":"<<s.max
     <<",\"median\":"<<s.median
     <<",\"mad\":"<<s.mad<<"}";
}
static void write_crossover(std::ostream&o,double c){
    const double ws=hstar(c,kDeltaWS),wc=hstar(c,kDeltaWC);
    o<<"{\"W-S\":{\"H_star\":"<<std::setprecision(15)<<ws
     <<",\"strict_integer_tokens\":"<<strict_tokens(ws)<<"},"
     <<"\"W-C\":{\"H_star\":"<<wc
     <<",\"strict_integer_tokens\":"<<strict_tokens(wc)<<"}}";
}

}

int main(int argc,char**argv){
    std::string model,shader_dir,sidecar,out="B1_2_PERFORMANCE_RESULT.json";
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            auto need=[&](const char*f){
                if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);
                return std::string(argv[++i]);
            };
            if(a=="--model")model=need("--model");
            else if(a=="--shader-dir")shader_dir=need("--shader-dir");
            else if(a=="--sidecar")sidecar=need("--sidecar");
            else if(a=="--out")out=need("--out");
        }
        if(model.empty()||shader_dir.empty()||sidecar.empty())
            throw std::runtime_error("--model, --shader-dir, --sidecar are required");
        if(std::filesystem::file_size(sidecar)!=kExecFamilyBytes)
            throw std::runtime_error("B1.2 performance sidecar byte count mismatch");
        if(file_sha256_stream(sidecar)!=kExpectedSidecarRawSha)
            throw std::runtime_error("B1.2 performance sidecar raw SHA256 mismatch");

        GgufInfo gguf=GgufReader(model).read();
        TensorStore store;
        TensorStoreReport ts=store.inspect(model,gguf);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("B1.2 performance TensorStore invariants failed");
        const uint8_t*payload=store.mapped_base()+gguf.data_offset;

        std::array<const uint8_t*,14>src_cpu{};
        std::array<const GgufTensorInfo*,14>targets{};
        for(uint32_t slot=0;slot<14u;++slot){
            const auto*t=find_tensor(gguf,target_name(kLayers[slot]));
            if(!t||t->ggml_type!=12u||t->dims.size()<2u||t->dims[0]!=kFFN||t->dims[1]!=kHidden)
                throw std::runtime_error("B1.2 performance target tensor mismatch: "+target_name(kLayers[slot]));
            if(tensor_nbytes(*t)!=kSourceLayerBytes)
                throw std::runtime_error("B1.2 performance target tensor bytes mismatch");
            src_cpu[slot]=payload+t->offset;
            targets[slot]=t;
        }

        VkRuntime vk;
        vk.init();
        std::vector<Buffer>arenas;
        arenas.reserve(kFrozenWeightArenas.size());
        uint64_t arena_bytes=0;
        for(const auto&a:kFrozenWeightArenas){
            const uint64_t bytes=a.end-a.start;
            arena_bytes+=bytes;
            arenas.push_back(vk.make_buffer(bytes,payload+a.start));
        }
        if(arena_bytes!=4677120000ull)
            throw std::runtime_error("B1.2 performance frozen arena byte total mismatch");

        std::array<Buffer*,14>src_gpu{};
        std::array<uint32_t,14>src_base{};
        for(uint32_t slot=0;slot<14u;++slot){
            const uint64_t s=targets[slot]->offset,e=s+kSourceLayerBytes;
            bool found=false;
            for(uint32_t ai=0;ai<uint32_t(kFrozenWeightArenas.size());++ai){
                const auto&a=kFrozenWeightArenas[ai];
                if(s>=a.start&&e<=a.end){
                    const uint64_t base=s-a.start;
                    if(base>0xffffffffull)throw std::runtime_error("B1.2 performance source base exceeds uint32");
                    src_gpu[slot]=&arenas[ai];
                    src_base[slot]=uint32_t(base);
                    found=true;
                    break;
                }
            }
            if(!found)throw std::runtime_error("B1.2 performance target not contained in frozen arena");
        }

        Buffer exec=vk.make_buffer(kExecFamilyBytes,nullptr);
        const VkMemoryPropertyFlags required_mem=
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if((vk.memory_type_flags()&required_mem)!=required_mem)
            throw std::runtime_error("B1.2 performance frozen UMA flags missing");

        auto p1_ops=build_p1_ops(src_gpu,src_base,exec,shader_dir);
        if(p1_ops.size()!=14u)throw std::runtime_error("B1.2 P1 performance dispatch count mismatch");
        auto p1_chain=vk.prepare_chain(p1_ops);

        PerfState p1,warm,cold;
        p1.samples.reserve(kB12Attempts);
        warm.samples.reserve(kB12Attempts);
        cold.samples.reserve(kB12Attempts);

        uint32_t frozen_sector=0;
        for(uint32_t block=0;block<kB12Attempts;++block){
            // Fixed pre-registered block order: P1 -> P3-WARM -> P3-COLD.
            std::memset(exec.mapped,0,size_t(exec.size));
            ChainStats ps=vk.execute_prepared(p1_chain,p1_ops,false);
            p1.samples.push_back(ps.record_submit_wait_ms);
            if(!validate_attempt(vk,src_cpu,src_gpu,src_base,exec,shader_dir))
                throw std::runtime_error("B1.2 P1 post-attempt correctness failure");

            buffered_preload(sidecar);
            std::memset(exec.mapped,0,size_t(exec.size));
            auto wt0=std::chrono::steady_clock::now();
            buffered_load(sidecar,reinterpret_cast<uint8_t*>(exec.mapped));
            auto wt1=std::chrono::steady_clock::now();
            warm.samples.push_back(std::chrono::duration<double,std::milli>(wt1-wt0).count());
            if(!validate_attempt(vk,src_cpu,src_gpu,src_base,exec,shader_dir))
                throw std::runtime_error("B1.2 P3-WARM post-attempt correctness failure");

            std::memset(exec.mapped,0,size_t(exec.size));
            uint32_t sector=0;
            auto ct0=std::chrono::steady_clock::now();
            unbuffered_load(sidecar,reinterpret_cast<uint8_t*>(exec.mapped),sector);
            auto ct1=std::chrono::steady_clock::now();
            if(block==0u)frozen_sector=sector;
            if(sector!=frozen_sector)throw std::runtime_error("B1.2 P3-COLD sector size drift");
            cold.samples.push_back(std::chrono::duration<double,std::milli>(ct1-ct0).count());
            if(!validate_attempt(vk,src_cpu,src_gpu,src_base,exec,shader_dir))
                throw std::runtime_error("B1.2 P3-COLD post-attempt correctness failure");
        }

        PerfSummary s1=summarize_b12(p1.samples);
        PerfSummary sw=summarize_b12(warm.samples);
        PerfSummary sc=summarize_b12(cold.samples);

        // Results are emitted only after all 24 attempts and all correctness checks pass.
        std::ofstream o(out,std::ios::binary|std::ios::trunc);
        if(!o)throw std::runtime_error("cannot write B1.2 performance result");
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.v1.b1_2.p1_p3.performance.v0.1\",\n";
        o<<"  \"status\":\"PASS_COMPLETE_24_ATTEMPT_COLLECTION\",\n";
        o<<"  \"attempt_design\":{\"blocks\":8,\"order_per_block\":[\"P1\",\"P3-WARM\",\"P3-COLD\"],\"warmup_attempts\":0,\"candidate_specific_retries\":false},\n";
        o<<"  \"frozen_reference\":{\"P0_ms\":"<<std::setprecision(15)<<kP0ReferenceMs
         <<",\"interpretation\":\"canonical_fixed_reference_not_population_estimate\"},\n";
        o<<"  \"P1\":{\"primary_metric\":\"record_submit_wait_ms\",\"samples_ms\":";
        write_samples(o,p1.samples);o<<",\"summary_ms\":";write_summary(o,s1);
        o<<",\"correctness_all_attempts\":true,\"beats_P0_reference\":"<<(s1.median<kP0ReferenceMs?"true":"false")
         <<",\"crossover\":";write_crossover(o,s1.median);o<<"},\n";
        o<<"  \"P3_WARM\":{\"primary_metric\":\"wall_ms_buffered_load\",\"samples_ms\":";
        write_samples(o,warm.samples);o<<",\"summary_ms\":";write_summary(o,sw);
        o<<",\"correctness_all_attempts\":true,\"beats_P0_reference\":"<<(sw.median<kP0ReferenceMs?"true":"false")
         <<",\"crossover\":";write_crossover(o,sw.median);o<<"},\n";
        o<<"  \"P3_COLD\":{\"primary_metric\":\"wall_ms_unbuffered_load\",\"samples_ms\":";
        write_samples(o,cold.samples);o<<",\"summary_ms\":";write_summary(o,sc);
        o<<",\"correctness_all_attempts\":true,\"sector_bytes\":"<<frozen_sector
         <<",\"chunk_bytes\":"<<kP3ChunkBytes
         <<",\"beats_P0_reference\":"<<(sc.median<kP0ReferenceMs?"true":"false")
         <<",\"crossover\":";write_crossover(o,sc.median);o<<"},\n";
        o<<"  \"integrity\":{\"exec148_bytes\":"<<kExecFamilyBytes
         <<",\"canonical_family_sha256\":\""<<kCanonicalFamilyHash
         <<"\",\"raw_exec148_sha256\":\""<<kExpectedSidecarRawSha
         <<"\",\"all_attempts_validated_outside_primary_timer\":true},\n";
        o<<"  \"scope\":{\"A_retest\":false,\"B_steady_state_retest\":false,\"AB_retest\":false,\"overlap\":false,\"posthoc_tuning\":false}\n";
        o<<"}\n";
        o.close();

        vk.destroy_prepared(p1_chain);
        for(auto&b:arenas)vk.destroy_buffer(b);
        vk.destroy_buffer(exec);
        std::cout<<"B1_2_PERFORMANCE_COLLECTION=PASS_COMPLETE_24_ATTEMPT_COLLECTION\n";
        std::cout<<"RESULT="<<out<<"\n";
        return 0;
    }catch(const std::exception&e){
        // Never emit partial timing samples on failure.
        std::cerr<<"B1.2 PERFORMANCE STOP: "<<e.what()<<"\n";
        return 2;
    }
}
