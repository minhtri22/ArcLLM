#include "llama.h"
#include <algorithm>
#include <cctype>
#include <chrono>
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
    if(!text)return;
    std::lock_guard<std::mutex> lk(g_log_mu);
    g_log += text;
}

static uint64_t abs_ns(const std::chrono::steady_clock::time_point& t){
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count());
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
    if(w=="W-S")return {1,133151,133152,152062};
    if(w=="W-C"){
        std::vector<llama_token> v(256);
        v[0]=1;v[1]=133151;v[2]=133152;v[3]=152062;
        for(uint32_t i=4;i<256;++i)v[i]=llama_token(1u+((104729u+7919u*i)%152063u));
        return v;
    }
    throw std::runtime_error("CORE0E workload must be W-S or W-C");
}

static std::vector<llama_token> forced_decode(const std::string& w){
    if(w=="W-S")return {
        136406,144325,181,8100,16019,23938,31857,39776,47695,55614,63533,
        71452,79371,87290,95209,103128,111047,118966,126885,134804,142723,
        150642,6498,14417,22336,30255,38174,46093,54012,61931,69850
    };
    if(w=="W-C")return {
        3112,11031,18950,26869,34788,42707,50626,58545,66464,74383,82302,
        90221,98140,106059,113978,121897,129816,137735,145654,1510,9429,
        17348,25267,33186,41105,49024,56943,64862,72781,80700,88619
    };
    throw std::runtime_error("CORE0E workload must be W-S or W-C");
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
    std::string model_path,workload,out_path="core0e_llama_result.json",phase_out;
    bool self_test=false;
    llama_model * model=nullptr;
    llama_context * ctx=nullptr;
    bool backend_initialized=false;
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
            else if(a=="--phase-out")phase_out=need("--phase-out");
            else if(a=="--self-test")self_test=true;
            else throw std::runtime_error("unknown argument: "+a);
        }

        if(self_test){
            for(const std::string w:{"W-S","W-C"}){
                const auto p=frozen_prompt(w);
                const auto f=forced_decode(w);
                if((w=="W-S"&&p.size()!=4u)||(w=="W-C"&&p.size()!=256u)||f.size()!=31u)
                    throw std::runtime_error("CORE0E self-test cardinality");
                for(auto t:f)if(t<0||t>=152064)throw std::runtime_error("CORE0E self-test forced token range");
            }
            std::cout<<"CORE0E_LLAMA_COMBINED_ADAPTER_SELF_TEST=PASS\n";
            return 0;
        }

        if(model_path.empty()||workload.empty()||phase_out.empty())
            throw std::runtime_error("required: --model --workload --phase-out");

        const auto prompt=frozen_prompt(workload);
        const auto forced=forced_decode(workload);
        const std::string prompt_hash=hex64(fnv1a64(prompt.data(),prompt.size()*sizeof(llama_token)));

        const auto child_start=std::chrono::steady_clock::now();

        llama_log_set(log_cb,nullptr);
        ggml_backend_load_all();
        llama_backend_init();
        backend_initialized=true;

        llama_model_params mp=llama_model_default_params();
        mp.n_gpu_layers=-1;
        model=llama_model_load_from_file(model_path.c_str(),mp);
        if(!model)throw std::runtime_error("llama_model_load_from_file failed");
        const llama_vocab * vocab=llama_model_get_vocab(model);
        if(!vocab)throw std::runtime_error("llama_model_get_vocab failed");
        const int32_t n_vocab=llama_vocab_n_tokens(vocab);
        if(n_vocab!=152064)throw std::runtime_error("CORE0E baseline vocab mismatch");
        if(llama_model_n_layer(model)!=28)throw std::runtime_error("CORE0E baseline layer count mismatch");

        llama_context_params cp=llama_context_default_params();
        cp.n_ctx=4096;cp.n_batch=256;cp.n_ubatch=256;cp.n_seq_max=1;
        cp.n_threads=8;cp.n_threads_batch=8;cp.type_k=GGML_TYPE_F32;cp.type_v=GGML_TYPE_F32;
        cp.offload_kqv=true;cp.no_perf=false;
        ctx=llama_init_from_model(model,cp);
        if(!ctx)throw std::runtime_error("llama_init_from_model failed");
        if(llama_n_ctx(ctx)!=4096u||llama_n_batch(ctx)!=256u||llama_n_ubatch(ctx)!=256u)
            throw std::runtime_error("CORE0E baseline context mismatch");

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

        llama_perf_context_reset(ctx);
        std::vector<llama_token> predicted;
        predicted.reserve(32);
        bool finite=true;
        std::string error;

        const auto prefill_start=std::chrono::steady_clock::now();
        llama_batch batch=llama_batch_get_one(const_cast<llama_token*>(prompt.data()),int32_t(prompt.size()));
        int rc=llama_decode(ctx,batch);
        if(rc!=0)throw std::runtime_error("llama_decode prefill rc="+std::to_string(rc));
        GreedyScan s=scan_logits(llama_get_logits_ith(ctx,-1),n_vocab);
        if(!s.finite)throw std::runtime_error("prefill logits non-finite");
        predicted.push_back(s.top1);
        const auto prefill_end=std::chrono::steady_clock::now();

        const auto decode_start=std::chrono::steady_clock::now();
        for(size_t di=0;di<forced.size();++di){
            llama_token input=forced[di];
            batch=llama_batch_get_one(&input,1);
            rc=llama_decode(ctx,batch);
            if(rc!=0)throw std::runtime_error("llama_decode cached rc="+std::to_string(rc));
            s=scan_logits(llama_get_logits_ith(ctx,-1),n_vocab);
            if(!s.finite)throw std::runtime_error("decode logits non-finite");
            predicted.push_back(s.top1);
        }
        const auto decode_end=std::chrono::steady_clock::now();

        const auto perf=llama_perf_context(ctx);
        const bool success=finite&&predicted.size()==32u&&full_offload&&perf.n_p_eval==int32_t(prompt.size())&&perf.n_eval==31;

        llama_free(ctx); ctx=nullptr;
        llama_model_free(model); model=nullptr;
        llama_backend_free(); backend_initialized=false;
        const auto child_end=std::chrono::steady_clock::now();

        std::ofstream o(out_path,std::ios::binary|std::ios::trunc);
        if(!o)throw std::runtime_error("cannot write --out");
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.core0e.llama_teacher_forced.v0.1\",\n";
        o<<"  \"system\":\"llama.cpp\",\n";
        o<<"  \"baseline_release\":\"v0.4.1\",\n";
        o<<"  \"baseline_commit\":\"b29c606e28a01b1bc8c1351026a0fa6e616bf6c4\",\n";
        o<<"  \"workload\":\""<<workload<<"\",\n";
        o<<"  \"prompt_tokens\":"<<prompt.size()<<",\n";
        o<<"  \"prompt_hash_fnv1a64\":\""<<prompt_hash<<"\",\n";
        o<<"  \"teacher_forced\":true,\n";
        o<<"  \"success\":"<<(success?"true":"false")<<",\n";
        o<<"  \"final_logits_finite\":"<<(finite?"true":"false")<<",\n";
        o<<"  \"predicted_token_ids\":[";
        for(size_t i=0;i<predicted.size();++i){if(i)o<<",";o<<predicted[i];}
        o<<"],\n";
        o<<"  \"forced_decode_input_ids\":[";
        for(size_t i=0;i<forced.size();++i){if(i)o<<",";o<<forced[i];}
        o<<"],\n";
        o<<"  \"resolved\":{\"n_ctx\":4096,\"n_batch\":256,\"n_ubatch\":256,\"threads\":8,\"threads_batch\":8,\"kv_k\":\"F32\",\"kv_v\":\"F32\",\"n_gpu_layers_requested\":-1},\n";
        o<<"  \"runtime\":{\"vulkan_log_present\":"<<(vulkan_log?"true":"false")
         <<",\"offload_reported\":"<<(offload_reported?"true":"false")
         <<",\"offloaded_layers\":"<<off_num
         <<",\"offloaded_layers_total\":"<<off_den
         <<",\"full_offload\":"<<(full_offload?"true":"false")<<"},\n";
        o<<"  \"perf\":{\"t_load_ms\":"<<perf.t_load_ms
         <<",\"t_p_eval_ms\":"<<perf.t_p_eval_ms
         <<",\"t_eval_ms\":"<<perf.t_eval_ms
         <<",\"n_p_eval\":"<<perf.n_p_eval
         <<",\"n_eval\":"<<perf.n_eval
         <<",\"n_reused\":"<<perf.n_reused<<"},\n";
        o<<"  \"error\":\""<<esc(error)<<"\"\n";
        o<<"}\n";
        o.close();

        std::ofstream po(phase_out,std::ios::binary|std::ios::trunc);
        if(!po)throw std::runtime_error("cannot write --phase-out");
        po<<"{\n";
        po<<"  \"schema\":\"arcllm.core0e.phase_markers.v0.1\",\n";
        po<<"  \"timing_authority\":\"DIAGNOSTIC_ONLY\",\n";
        po<<"  \"clock\":\"std::chrono::steady_clock\",\n";
        po<<"  \"child_science_window_start_ns\":"<<abs_ns(child_start)<<",\n";
        po<<"  \"prefill_wall_start_ns\":"<<abs_ns(prefill_start)<<",\n";
        po<<"  \"prefill_wall_end_ns\":"<<abs_ns(prefill_end)<<",\n";
        po<<"  \"decode_wall_start_ns\":"<<abs_ns(decode_start)<<",\n";
        po<<"  \"decode_wall_end_ns\":"<<abs_ns(decode_end)<<",\n";
        po<<"  \"child_science_window_end_ns\":"<<abs_ns(child_end)<<"\n";
        po<<"}\n";
        po.close();

        return success?0:3;
    }catch(const std::exception& e){
        if(ctx)llama_free(ctx);
        if(model)llama_model_free(model);
        if(backend_initialized)llama_backend_free();
        std::cerr<<"CORE0E llama adapter error: "<<e.what()<<"\n";
        std::ofstream o(out_path,std::ios::binary|std::ios::trunc);
        if(o)o<<"{\n  \"schema\":\"arcllm.core0e.llama_teacher_forced.v0.1\",\n  \"success\":false,\n  \"error\":\""<<esc(e.what())<<"\"\n}\n";
        return 2;
    }
}
