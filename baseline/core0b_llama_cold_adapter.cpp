#include "llama.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::mutex g_log_mu;
static std::string g_log;

static void log_cb(enum ggml_log_level,const char * text,void *){
    if(!text) return;
    std::lock_guard<std::mutex> lk(g_log_mu);
    g_log += text;
}

static uint64_t fnv1a64(const void * data,size_t n){
    const uint8_t * p=reinterpret_cast<const uint8_t*>(data);
    uint64_t h=1469598103934665603ull;
    for(size_t i=0;i<n;++i){h^=uint64_t(p[i]);h*=1099511628211ull;}
    return h;
}

static std::string hex64(uint64_t v){
    std::ostringstream ss;
    ss<<std::hex<<std::setw(16)<<std::setfill('0')<<v;
    return ss.str();
}

static std::string esc(const std::string& s){
    std::string o;
    for(char c:s){
        if(c=='\\'||c=='"'){o+='\\';o+=c;}
        else if(c=='\n')o+="\\n";
        else if(c=='\r')o+="\\r";
        else o+=c;
    }
    return o;
}

static std::vector<llama_token> frozen_prompt(const std::string& w){
    if(w=="W-S") return {1,133151,133152,152062};
    if(w=="W-C"){
        std::vector<llama_token> v(256);
        v[0]=1;v[1]=133151;v[2]=133152;v[3]=152062;
        for(uint32_t i=4;i<256;++i) v[i]=llama_token(1u+((104729u+7919u*i)%152063u));
        return v;
    }
    throw std::runtime_error("CORE0B workload must be W-S or W-C");
}

struct GreedyScan{
    llama_token top1=0;
    float logit1=-std::numeric_limits<float>::infinity();
    bool finite=true;
};

static GreedyScan scan_logits(const float * p,int32_t n){
    GreedyScan r;
    if(!p||n<=0){r.finite=false;return r;}
    for(int32_t i=0;i<n;++i){
        const float v=p[i];
        if(!std::isfinite(v)){r.finite=false;continue;}
        if(v>r.logit1){r.logit1=v;r.top1=llama_token(i);}
    }
    if(!std::isfinite(r.logit1))r.finite=false;
    return r;
}

int main(int argc,char ** argv){
    std::string model_path,workload,out_path="core0b_llama_result.json";
    bool qualify_only=false;
    try{
        for(int i=1;i<argc;++i){
            const std::string a=argv[i];
            auto need=[&](const char * flag){
                if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+flag);
                return std::string(argv[++i]);
            };
            if(a=="--model")model_path=need("--model");
            else if(a=="--workload")workload=need("--workload");
            else if(a=="--out")out_path=need("--out");
            else if(a=="--qualify-only")qualify_only=true;
            else throw std::runtime_error("unknown argument: "+a);
        }
        if(model_path.empty()||workload.empty())
            throw std::runtime_error("required: --model --workload");
        const auto prompt=frozen_prompt(workload);
        const std::string prompt_hash=hex64(fnv1a64(prompt.data(),prompt.size()*sizeof(llama_token)));

        llama_log_set(log_cb,nullptr);
        ggml_backend_load_all();
        llama_backend_init();

        llama_model_params mp=llama_model_default_params();
        mp.n_gpu_layers=-1;
        llama_model * model=llama_model_load_from_file(model_path.c_str(),mp);
        if(!model)throw std::runtime_error("llama_model_load_from_file failed");
        const llama_vocab * vocab=llama_model_get_vocab(model);
        if(!vocab)throw std::runtime_error("llama_model_get_vocab failed");
        const int32_t n_vocab=llama_vocab_n_tokens(vocab);
        if(n_vocab!=152064)throw std::runtime_error("CORE0B baseline vocab mismatch");
        if(llama_model_n_layer(model)!=28)throw std::runtime_error("CORE0B baseline layer count mismatch");

        llama_context_params cp=llama_context_default_params();
        cp.n_ctx=4096;
        cp.n_batch=256;
        cp.n_ubatch=256;
        cp.n_seq_max=1;
        cp.n_threads=8;
        cp.n_threads_batch=8;
        cp.type_k=GGML_TYPE_F32;
        cp.type_v=GGML_TYPE_F32;
        cp.offload_kqv=true;
        cp.no_perf=false;
        llama_context * ctx=llama_init_from_model(model,cp);
        if(!ctx)throw std::runtime_error("llama_init_from_model failed");
        if(llama_n_ctx(ctx)!=4096u||llama_n_batch(ctx)!=256u||llama_n_ubatch(ctx)!=256u)
            throw std::runtime_error("CORE0B baseline context mismatch");

        std::string runtime_log;
        {
            std::lock_guard<std::mutex> lk(g_log_mu);
            runtime_log=g_log;
        }
        std::string lower=runtime_log;
        std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});
        const bool vulkan_log=lower.find("vulkan")!=std::string::npos;
        std::smatch m;
        const std::regex re("offloaded[ ]+([0-9]+)/([0-9]+)[ ]+layers[ ]+to[ ]+GPU");
        const bool offload_reported=std::regex_search(runtime_log,m,re);
        const int off_num=offload_reported?std::stoi(m[1].str()):-1;
        const int off_den=offload_reported?std::stoi(m[2].str()):-1;
        const bool full_offload=offload_reported&&off_num==off_den&&off_num>0;

        std::vector<llama_token> generated;
        bool finite=true;
        bool success=false;
        std::string error;

        if(!qualify_only){
            try{
                llama_batch batch=llama_batch_get_one(const_cast<llama_token*>(prompt.data()),int32_t(prompt.size()));
                int rc=llama_decode(ctx,batch);
                if(rc!=0)throw std::runtime_error("llama_decode prefill rc="+std::to_string(rc));
                GreedyScan s=scan_logits(llama_get_logits_ith(ctx,-1),n_vocab);
                if(!s.finite)throw std::runtime_error("prefill logits non-finite");
                llama_token next=s.top1;
                generated.push_back(next);
                for(int di=0;di<31;++di){
                    batch=llama_batch_get_one(&next,1);
                    rc=llama_decode(ctx,batch);
                    if(rc!=0)throw std::runtime_error("llama_decode cached rc="+std::to_string(rc));
                    s=scan_logits(llama_get_logits_ith(ctx,-1),n_vocab);
                    if(!s.finite)throw std::runtime_error("decode logits non-finite");
                    next=s.top1;
                    generated.push_back(next);
                }
                finite=true;
                success=generated.size()==32u&&full_offload;
            }catch(const std::exception& e){
                finite=false;
                error=e.what();
                success=false;
            }
        }else{
            success=vulkan_log&&full_offload;
        }

        std::ofstream o(out_path,std::ios::binary|std::ios::trunc);
        if(!o)throw std::runtime_error("cannot write --out");
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.core0b.llama_cold_request.v0.1\",\n";
        o<<"  \"system\":\"llama.cpp\",\n";
        o<<"  \"baseline_release\":\"v0.4.1\",\n";
        o<<"  \"baseline_commit\":\"b29c606e28a01b1bc8c1351026a0fa6e616bf6c4\",\n";
        o<<"  \"workload\":\""<<workload<<"\",\n";
        o<<"  \"prompt_tokens\":"<<prompt.size()<<",\n";
        o<<"  \"prompt_hash_fnv1a64\":\""<<prompt_hash<<"\",\n";
        o<<"  \"qualify_only\":"<<(qualify_only?"true":"false")<<",\n";
        o<<"  \"success\":"<<(success?"true":"false")<<",\n";
        o<<"  \"final_logits_finite\":"<<(finite?"true":"false")<<",\n";
        o<<"  \"generated_token_ids\":[";
        for(size_t i=0;i<generated.size();++i){if(i)o<<",";o<<generated[i];}
        o<<"],\n";
        o<<"  \"generated_hash_fnv1a64\":\""<<hex64(fnv1a64(generated.data(),generated.size()*sizeof(llama_token)))<<"\",\n";
        o<<"  \"resolved\":{\"n_ctx\":"<<llama_n_ctx(ctx)
         <<",\"n_batch\":"<<llama_n_batch(ctx)
         <<",\"n_ubatch\":"<<llama_n_ubatch(ctx)
         <<",\"threads\":8,\"threads_batch\":8,\"kv_k\":\"F32\",\"kv_v\":\"F32\",\"n_gpu_layers_requested\":-1},\n";
        o<<"  \"runtime\":{\"vulkan_log_present\":"<<(vulkan_log?"true":"false")
         <<",\"offload_reported\":"<<(offload_reported?"true":"false")
         <<",\"offloaded_layers\":"<<off_num
         <<",\"offloaded_layers_total\":"<<off_den
         <<",\"full_offload\":"<<(full_offload?"true":"false")<<"},\n";
        o<<"  \"error\":\""<<esc(error)<<"\"\n";
        o<<"}\n";
        o.close();

        llama_free(ctx);
        llama_model_free(model);
        llama_backend_free();
        return success?0:3;
    }catch(const std::exception& e){
        std::ofstream o(out_path,std::ios::binary|std::ios::trunc);
        if(o)o<<"{\n  \"schema\":\"arcllm.core0b.llama_cold_request.v0.1\",\n  \"success\":false,\n  \"error\":\""<<esc(e.what())<<"\"\n}\n";
        std::cerr<<"CORE0B llama adapter error: "<<e.what()<<"\n";
        return 2;
    }
}
