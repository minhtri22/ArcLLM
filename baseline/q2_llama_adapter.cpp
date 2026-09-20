#include "llama.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
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
static void q2_log_callback(enum ggml_log_level,const char * text,void *){
    if(!text)return;
    {std::lock_guard<std::mutex> lk(g_log_mu);g_log+=text;}
    std::cerr<<text<<std::flush;
}
static uint64_t fnv1a64(const void * data,size_t n){
    const uint8_t * p=reinterpret_cast<const uint8_t*>(data);
    uint64_t h=1469598103934665603ull;
    for(size_t i=0;i<n;++i){h^=uint64_t(p[i]);h*=1099511628211ull;}
    return h;
}
static std::string hex64(uint64_t v){
    std::ostringstream ss;ss<<std::hex<<std::setw(16)<<std::setfill('0')<<v;return ss.str();
}
static std::string esc(const std::string&s){
    std::string o;for(char c:s){if(c=='\\'||c=='"'){o+='\\';o+=c;}else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else o+=c;}return o;
}
struct Q2GreedyScan{llama_token token=0;bool finite=false;};
static Q2GreedyScan q2_greedy_finite(const float * p,int32_t n){
    Q2GreedyScan r;
    if(!p||n<=0)return r;
    float best=-std::numeric_limits<float>::infinity();
    bool have=false,all_finite=true;
    for(int32_t i=0;i<n;++i){
        const float v=p[i];
        if(!std::isfinite(v)){all_finite=false;continue;}
        if(!have||v>best){best=v;r.token=llama_token(i);have=true;}
    }
    r.finite=all_finite&&have;
    return r;
}
struct Attempt{
    int index=-1;bool warmup=false,success=false,final_logits_finite=false;
    double ttft_ms=0,decode_ms=0,e2e_ms=0,decode_tps=0;
    std::vector<llama_token> generated;
    std::string generated_hash,final_logits_hash,error;
};
static std::vector<llama_token> frozen_prompt(const std::string&w){
    if(w=="W-S")return {1,133151,133152,152062};
    if(w=="W-C"){
        std::vector<llama_token> v(256);
        v[0]=1;v[1]=133151;v[2]=133152;v[3]=152062;
        for(uint32_t i=4;i<256;++i)v[i]=llama_token(1u+((104729u+7919u*i)%152063u));
        return v;
    }
    throw std::runtime_error("Q2 baseline workload must be W-S or W-C");
}
int main(int argc,char ** argv){
    std::string model_path,workload,out="q2_baseline_cell.json";
    int warmups=1,measured=5;
    bool qualify_only=false;
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            auto need=[&](const char*f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};
            if(a=="--model")model_path=need("--model");
            else if(a=="--workload")workload=need("--workload");
            else if(a=="--warmups")warmups=std::stoi(need("--warmups"));
            else if(a=="--measured")measured=std::stoi(need("--measured"));
            else if(a=="--out")out=need("--out");
            else if(a=="--qualify-only")qualify_only=true;
        }
        if(model_path.empty()||workload.empty())throw std::runtime_error("Q2 baseline required arguments missing");
        if(warmups!=1||measured!=5)throw std::runtime_error("Q2 baseline frozen repetition count mismatch");
        const auto prompt=frozen_prompt(workload);
        const std::string prompt_hash=hex64(fnv1a64(prompt.data(),prompt.size()*sizeof(llama_token)));

        auto setup0=std::chrono::steady_clock::now();
        llama_log_set(q2_log_callback,nullptr);
        ggml_backend_load_all();
        llama_backend_init();

        llama_model_params mp=llama_model_default_params();
        mp.n_gpu_layers=-1;
        llama_model * model=llama_model_load_from_file(model_path.c_str(),mp);
        if(!model)throw std::runtime_error("llama_model_load_from_file failed");
        const llama_vocab * vocab=llama_model_get_vocab(model);
        if(!vocab)throw std::runtime_error("llama_model_get_vocab failed");
        const int32_t n_vocab=llama_vocab_n_tokens(vocab);
        if(n_vocab!=152064)throw std::runtime_error("Q2 baseline vocab mismatch");
        if(llama_model_n_layer(model)!=28)throw std::runtime_error("Q2 baseline layer count mismatch");

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
            throw std::runtime_error("Q2 baseline resolved context mismatch");

        std::string runtime_log;
        {std::lock_guard<std::mutex> lk(g_log_mu);runtime_log=g_log;}
        std::string runtime_lower=runtime_log;std::transform(runtime_lower.begin(),runtime_lower.end(),runtime_lower.begin(),[](unsigned char c){return char(std::tolower(c));});
        const bool vulkan_log=runtime_lower.find("vulkan")!=std::string::npos;
        std::smatch off_match;
        const std::regex off_re("offloaded[ ]+([0-9]+)/([0-9]+)[ ]+layers[ ]+to[ ]+GPU");
        const bool offload_reported=std::regex_search(runtime_log,off_match,off_re);
        const int off_num=offload_reported?std::stoi(off_match[1].str()):-1;
        const int off_den=offload_reported?std::stoi(off_match[2].str()):-1;
        const bool full_offload=offload_reported&&off_num==off_den&&off_num>0;

        if(qualify_only){
            std::ofstream q(out,std::ios::binary);if(!q)throw std::runtime_error("cannot write Q2 baseline runtime qualification JSON");
            q<<"{\n  \"schema\":\"arcllm.q2.baseline_runtime_qualification.v1\",\n";
            q<<"  \"status\":\""<<((vulkan_log&&full_offload)?"QUALIFIED":"NOT_MATCHED")<<"\",\n";
            q<<"  \"baseline_release\":\"v0.4.1\",\"baseline_commit\":\"391fac16460f15233a7740550d858ac96df3419d\",\n";
            q<<"  \"workload\":\""<<workload<<"\",\"prompt_tokens\":"<<prompt.size()<<",\"prompt_hash_fnv1a64\":\""<<prompt_hash<<"\",\n";
            q<<"  \"raw_token_input\":true,\"tokenizer_used\":false,\"chat_template_used\":false,\n";
            q<<"  \"resolved\":{\"n_ctx\":"<<llama_n_ctx(ctx)<<",\"n_batch\":"<<llama_n_batch(ctx)<<",\"n_ubatch\":"<<llama_n_ubatch(ctx)<<",\"threads\":8,\"threads_batch\":8,\"n_gpu_layers_requested\":-1,\"kv_k\":\"F32\",\"kv_v\":\"F32\",\"vocab\":"<<n_vocab<<",\"layers\":"<<llama_model_n_layer(model)<<"},\n";
            q<<"  \"runtime\":{\"vulkan_log_present\":"<<(vulkan_log?"true":"false")<<",\"offload_reported\":"<<(offload_reported?"true":"false")<<",\"offloaded_layers\":"<<off_num<<",\"offloaded_layers_total\":"<<off_den<<",\"full_offload\":"<<(full_offload?"true":"false")<<"},\n";
            q<<"  \"decode_executed\":false,\"measured_attempts\":0,\"runtime_log\":\""<<esc(runtime_log)<<"\"\n}\n";
            q.close();
            llama_free(ctx);llama_model_free(model);llama_backend_free();
            return (vulkan_log&&full_offload)?0:3;
        }

        auto run_one=[&](int index,bool warmup)->Attempt{
            Attempt a;a.index=index;a.warmup=warmup;
            llama_memory_clear(llama_get_memory(ctx),true);
            const std::string kind=warmup?"WARMUP":"ATTEMPT";
            std::cout<<"Q2_"<<kind<<"_BEGIN|llama.cpp|"<<workload<<"|"<<index<<"\n"<<std::flush;
            try{
                std::vector<llama_token> in=prompt;
                auto t0=std::chrono::steady_clock::now();
                llama_batch batch=llama_batch_get_one(in.data(),int32_t(in.size()));
                int rc=llama_decode(ctx,batch);
                if(rc!=0)throw std::runtime_error("llama_decode prefill rc="+std::to_string(rc));
                float * logits=llama_get_logits_ith(ctx,-1);
                const Q2GreedyScan prefill_scan=q2_greedy_finite(logits,n_vocab);
                if(!prefill_scan.finite)throw std::runtime_error("baseline prefill logits non-finite");
                llama_token next=prefill_scan.token;
                auto t1=std::chrono::steady_clock::now();
                a.ttft_ms=std::chrono::duration<double,std::milli>(t1-t0).count();
                a.generated.push_back(next);

                auto td0=std::chrono::steady_clock::now();
                for(int di=0;di<31;++di){
                    batch=llama_batch_get_one(&next,1);
                    rc=llama_decode(ctx,batch);
                    if(rc!=0)throw std::runtime_error("llama_decode cached rc="+std::to_string(rc));
                    logits=llama_get_logits_ith(ctx,-1);
                    const Q2GreedyScan decode_scan=q2_greedy_finite(logits,n_vocab);
                    if(!decode_scan.finite)throw std::runtime_error("baseline decode logits non-finite");
                    next=decode_scan.token;
                    a.generated.push_back(next);
                }
                auto td1=std::chrono::steady_clock::now();
                a.decode_ms=std::chrono::duration<double,std::milli>(td1-td0).count();
                a.e2e_ms=std::chrono::duration<double,std::milli>(td1-t0).count();
                a.decode_tps=31.0/(a.decode_ms/1000.0);
                logits=llama_get_logits_ith(ctx,-1);
                a.final_logits_finite=true;
                a.final_logits_hash=hex64(fnv1a64(logits,size_t(n_vocab)*sizeof(float)));
                a.generated_hash=hex64(fnv1a64(a.generated.data(),a.generated.size()*sizeof(llama_token)));
                a.success=a.final_logits_finite&&a.generated.size()==32u;
                if(!a.success)a.error="baseline attempt invariant failure";
            }catch(const std::exception&e){a.error=e.what();a.success=false;}
            std::cout<<"Q2_"<<kind<<"_END|llama.cpp|"<<workload<<"|"<<index<<"|"<<(a.success?"PASS":"FAIL")<<"\n"<<std::flush;
            return a;
        };

        const double setup_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-setup0).count();
        Attempt warm=run_one(0,true);
        std::vector<Attempt> attempts;for(int i=0;i<5;++i)attempts.push_back(run_one(i,false));

        char desc[512]={0};llama_model_desc(model,desc,sizeof(desc));
        std::ofstream o(out,std::ios::binary);if(!o)throw std::runtime_error("cannot write Q2 baseline JSON");
        o<<std::setprecision(15);
        o<<"{\n  \"schema\":\"arcllm.q2.llama_baseline_cell.v1\",\n";
        o<<"  \"system\":\"llama.cpp\",\"baseline_release\":\"v0.4.1\",\"baseline_commit\":\"391fac16460f15233a7740550d858ac96df3419d\",\"backend_requested\":\"Vulkan\",\n";
        o<<"  \"workload\":\""<<workload<<"\",\"prompt_tokens\":"<<prompt.size()<<",\"output_tokens\":32,\"prompt_hash_fnv1a64\":\""<<prompt_hash<<"\",\n";
        o<<"  \"resolved\":{\"n_ctx\":"<<llama_n_ctx(ctx)<<",\"n_batch\":"<<llama_n_batch(ctx)<<",\"n_ubatch\":"<<llama_n_ubatch(ctx)<<",\"threads\":8,\"threads_batch\":8,\"n_gpu_layers_requested\":-1,\"kv_k\":\"F32\",\"kv_v\":\"F32\",\"offload_kqv\":true,\"vocab\":"<<n_vocab<<",\"layers\":"<<llama_model_n_layer(model)<<"},\n";
        o<<"  \"model_desc\":\""<<esc(desc)<<"\",\"setup_ms_descriptive\":"<<setup_ms<<",\n";
        auto emit=[&](const Attempt&a){
            o<<"{\"index\":"<<a.index<<",\"warmup\":"<<(a.warmup?"true":"false")<<",\"success\":"<<(a.success?"true":"false")
             <<",\"ttft_ms\":"<<a.ttft_ms<<",\"decode_ms\":"<<a.decode_ms<<",\"decode_tps\":"<<a.decode_tps<<",\"e2e_ms\":"<<a.e2e_ms
             <<",\"final_logits_finite\":"<<(a.final_logits_finite?"true":"false")<<",\"generated_token_ids\":[";
            for(size_t i=0;i<a.generated.size();++i){if(i)o<<",";o<<a.generated[i];}
            o<<"],\"generated_hash_fnv1a64\":\""<<a.generated_hash<<"\",\"final_logits_hash_fnv1a64\":\""<<a.final_logits_hash
             <<"\",\"final_state_checksum_available\":false,\"error\":\""<<esc(a.error)<<"\"}";
        };
        o<<"  \"warmup\":";emit(warm);o<<",\n  \"attempts\":[";
        for(size_t i=0;i<attempts.size();++i){if(i)o<<",";emit(attempts[i]);}
        o<<"],\n  \"governance\":{\"raw_token_input\":true,\"tokenizer_used\":false,\"chat_template_used\":false,\"eos_early_stop\":false,\"speculative_decoding\":false,\"q2_characterization_only\":true,\"advantage_claimed\":false}\n}\n";
        o.close();

        llama_free(ctx);
        llama_model_free(model);
        llama_backend_free();
        return 0;
    }catch(const std::exception&e){
        std::ofstream o(out,std::ios::binary);
        if(o)o<<"{\n  \"schema\":\"arcllm.q2.llama_baseline_cell.v1\",\n  \"status\":\"ERROR\",\n  \"error\":\""<<esc(e.what())<<"\"\n}\n";
        std::cerr<<"Q2 llama.cpp baseline error: "<<e.what()<<"\n";return 2;
    }
}
