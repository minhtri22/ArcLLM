#include "gguf.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr uint32_t F32=0, Q4_K=12, Q6_K=14;
constexpr uint32_t EXPECT_TENSORS=339, EXPECT_LAYERS=28, H=3584, KV=512, FFN=18944;
constexpr uint32_t EXPECT_F32=141, EXPECT_Q4=169, EXPECT_Q6=29;
constexpr uint32_t EXPECT_TARGET_Q4=28, EXPECT_TARGET_Q6=28;

const GgufTensorInfo* find_tensor(const GgufInfo& g,const std::string& name){
    for(const auto& t:g.tensors) if(t.name==name) return &t;
    return nullptr;
}
std::string type_name(uint32_t t){
    if(t==F32)return "F32";
    if(t==Q4_K)return "Q4_K";
    if(t==Q6_K)return "Q6_K";
    return "TYPE_"+std::to_string(t);
}
uint64_t row_bytes(uint32_t t,uint64_t n){
    if(t==F32)return n*4ull;
    if(n%256ull)throw std::runtime_error("quant row width not divisible by 256");
    if(t==Q4_K)return (n/256ull)*144ull;
    if(t==Q6_K)return (n/256ull)*210ull;
    throw std::runtime_error("unsupported tensor type "+std::to_string(t));
}
uint64_t rows_of(const GgufTensorInfo& t){
    if(t.dims.empty())throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t r=1;
    for(size_t i=1;i<t.dims.size();++i)r*=t.dims[i];
    return r;
}
uint64_t bytes_of(const GgufTensorInfo& t){return row_bytes(t.ggml_type,t.dims.at(0))*rows_of(t);}
void require_dims(const GgufTensorInfo* t,uint64_t d0,uint64_t d1,const std::string& name){
    if(!t)throw std::runtime_error("missing tensor: "+name);
    if(t->dims.size()!=2||t->dims[0]!=d0||t->dims[1]!=d1)throw std::runtime_error("unexpected dims for "+name);
    if(t->ggml_type!=Q4_K&&t->ggml_type!=Q6_K)throw std::runtime_error("unexpected quant type for "+name+": "+type_name(t->ggml_type));
}
std::string esc(const std::string& s){
    std::string o;
    for(char c:s){
        if(c=='\\'||c=='"'){o+='\\';o+=c;}
        else o+=c;
    }
    return o;
}
struct Pair{const GgufTensorInfo* v=nullptr;const GgufTensorInfo* d=nullptr;};
}

int main(int argc,char** argv){
    std::string model,out="arcllm_v1_m0_tensor_census.json";
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            if(a=="--model"&&i+1<argc)model=argv[++i];
            else if(a=="--out"&&i+1<argc)out=argv[++i];
            else throw std::runtime_error("usage: arcllm_v1_m0_tensor_census --model <gguf> [--out <json>]");
        }
        if(model.empty())throw std::runtime_error("--model required");
        GgufInfo g=GgufReader(model).read();
        if(g.tensor_count!=EXPECT_TENSORS||g.tensors.size()!=EXPECT_TENSORS)throw std::runtime_error("exact tensor census mismatch");
        auto bc=g.scalars.find("qwen2.block_count");
        auto he=g.scalars.find("qwen2.embedding_length");
        auto ar=g.scalars.find("general.architecture");
        if(bc==g.scalars.end()||std::stoul(bc->second)!=EXPECT_LAYERS)throw std::runtime_error("layer metadata mismatch");
        if(he==g.scalars.end()||std::stoul(he->second)!=H)throw std::runtime_error("hidden metadata mismatch");
        if(ar==g.scalars.end()||ar->second!="qwen2")throw std::runtime_error("architecture metadata mismatch");
        if(g.tensor_type_counts[F32]!=EXPECT_F32||g.tensor_type_counts[Q4_K]!=EXPECT_Q4||g.tensor_type_counts[Q6_K]!=EXPECT_Q6)
            throw std::runtime_error("authoritative quant-mix mismatch");

        std::vector<Pair> layers(EXPECT_LAYERS);
        uint32_t target_q4=0,target_q6=0;
        for(uint32_t l=0;l<EXPECT_LAYERS;++l){
            const std::string p="blk."+std::to_string(l);
            const std::string vn=p+".attn_v.weight";
            const std::string dn=p+".ffn_down.weight";
            layers[l].v=find_tensor(g,vn);
            layers[l].d=find_tensor(g,dn);
            require_dims(layers[l].v,H,KV,vn);
            require_dims(layers[l].d,FFN,H,dn);
            const GgufTensorInfo* items[2]={layers[l].v,layers[l].d};
            for(const auto* t:items){
                if(t->ggml_type==Q4_K)++target_q4;
                else if(t->ggml_type==Q6_K)++target_q6;
            }
        }
        const bool target_mix_pass=target_q4==EXPECT_TARGET_Q4&&target_q6==EXPECT_TARGET_Q6;
        const bool pass=target_mix_pass;

        std::ofstream o(out,std::ios::binary);
        if(!o)throw std::runtime_error("cannot open output JSON");
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.v1.m0.tensor_census.v0.1\",\n";
        o<<"  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";
        o<<"  \"execution_class\":\"METADATA_ONLY_NO_INFERENCE_NO_PERFORMANCE\",\n";
        o<<"  \"model\":{\"tensor_count\":"<<g.tensor_count<<",\"layers\":"<<EXPECT_LAYERS<<",\"hidden\":"<<H<<",\"kv_dim\":"<<KV<<",\"ffn\":"<<FFN<<"},\n";
        o<<"  \"authoritative_quant_mix\":{\"F32\":"<<g.tensor_type_counts[F32]<<",\"Q4_K\":"<<g.tensor_type_counts[Q4_K]<<",\"Q6_K\":"<<g.tensor_type_counts[Q6_K]<<"},\n";
        o<<"  \"target_summary\":{\"target_tensor_count\":56,\"Q4_K\":"<<target_q4<<",\"Q6_K\":"<<target_q6<<",\"expected_Q4_K\":28,\"expected_Q6_K\":28,\"pass\":"<<(target_mix_pass?"true":"false")<<"},\n";
        o<<"  \"layers\":[\n";
        for(uint32_t l=0;l<EXPECT_LAYERS;++l){
            const auto* v=layers[l].v;
            const auto* d=layers[l].d;
            o<<"    {\"layer\":"<<l<<",";
            o<<"\"attn_v\":{\"name\":\""<<esc(v->name)<<"\",\"type\":\""<<type_name(v->ggml_type)<<"\",\"ggml_type\":"<<v->ggml_type<<",\"dims\":["<<v->dims[0]<<","<<v->dims[1]<<"],\"row_bytes\":"<<row_bytes(v->ggml_type,v->dims[0])<<",\"tensor_bytes\":"<<bytes_of(*v)<<"},";
            o<<"\"ffn_down\":{\"name\":\""<<esc(d->name)<<"\",\"type\":\""<<type_name(d->ggml_type)<<"\",\"ggml_type\":"<<d->ggml_type<<",\"dims\":["<<d->dims[0]<<","<<d->dims[1]<<"],\"row_bytes\":"<<row_bytes(d->ggml_type,d->dims[0])<<",\"tensor_bytes\":"<<bytes_of(*d)<<"}}";
            if(l+1<EXPECT_LAYERS)o<<",";
            o<<"\n";
        }
        o<<"  ],\n";
        o<<"  \"patch_manifest\":[\n";
        bool first=true;
        for(uint32_t l=0;l<EXPECT_LAYERS;++l){
            const std::pair<std::string,const GgufTensorInfo*> items[2]={{"v_proj",layers[l].v},{"ffn_down",layers[l].d}};
            for(const auto& item:items){
                if(!first)o<<",\n";
                first=false;
                o<<"    {\"node\":\"L"<<(l<10?"0":"")<<l<<"."<<item.first<<"\",\"layer\":"<<l<<",\"dtype_quantization\":\""<<type_name(item.second->ggml_type)<<"\",\"tensor\":\""<<esc(item.second->name)<<"\"}";
            }
        }
        o<<"\n  ],\n";
        o<<"  \"gate\":{\"exact_56_target_tensors\":true,\"target_quant_mix_pass\":"<<(target_mix_pass?"true":"false")<<",\"no_inference\":true,\"no_performance_measurement\":true,\"m0_pass\":"<<(pass?"true":"false")<<"}\n";
        o<<"}\n";
        o.close();

        std::cout<<"M0 metadata-only tensor census\n";
        std::cout<<"target Q4_K="<<target_q4<<" Q6_K="<<target_q6<<"\n";
        for(uint32_t l=0;l<EXPECT_LAYERS;++l)
            std::cout<<"L"<<(l<10?"0":"")<<l<<" V="<<type_name(layers[l].v->ggml_type)<<" DOWN="<<type_name(layers[l].d->ggml_type)<<"\n";
        std::cout<<"M0 "<<(pass?"PASS":"FAIL")<<"\n";
        return pass?0:20;
    }catch(const std::exception& e){
        std::ofstream o(out,std::ios::binary);
        if(o)o<<"{\n  \"schema\":\"arcllm.v1.m0.tensor_census.v0.1\",\n  \"status\":\"ERROR\",\n  \"execution_class\":\"METADATA_ONLY_NO_INFERENCE_NO_PERFORMANCE\",\n  \"error\":\""<<esc(e.what())<<"\"\n}\n";
        std::cerr<<e.what()<<"\n";
        return 2;
    }
}
