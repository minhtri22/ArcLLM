#include "arcllm/v1/runtime.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<uint32_t> parse_u32_csv(const std::string& s){
    std::vector<uint32_t> out;
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss,item,',')){
        if(item.empty())throw std::runtime_error("empty token in CSV");
        unsigned long v=std::stoul(item);
        if(v>0xfffffffful)throw std::runtime_error("token exceeds uint32");
        out.push_back(static_cast<uint32_t>(v));
    }
    return out;
}

static std::string csv(const std::vector<uint32_t>& v){
    std::ostringstream o;
    for(size_t i=0;i<v.size();++i){if(i)o<<",";o<<v[i];}
    return o.str();
}

static void env_set(const char* k,const std::string& v){
#ifdef _WIN32
    if(_putenv_s(k,v.c_str())!=0)throw std::runtime_error(std::string("failed to set env ")+k);
#else
    if(setenv(k,v.c_str(),1)!=0)throw std::runtime_error(std::string("failed to set env ")+k);
#endif
}
static void env_clear(const char* k){
#ifdef _WIN32
    _putenv_s(k,"");
#else
    unsetenv(k);
#endif
}

int main(int argc,char**argv){
    std::string model,shader_dir,out,phase_out,trace_dir,run_id,mode;
    std::vector<uint32_t> input,forced;
    uint32_t profile=0;
    std::string timestamp_period,timestamp_bits;
    bool self_test=false;
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            auto need=[&](const char* f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};
            if(a=="--model")model=need("--model");
            else if(a=="--shader-dir")shader_dir=need("--shader-dir");
            else if(a=="--tokens")input=parse_u32_csv(need("--tokens"));
            else if(a=="--forced-decode-tokens")forced=parse_u32_csv(need("--forced-decode-tokens"));
            else if(a=="--profile")profile=static_cast<uint32_t>(std::stoul(need("--profile")));
            else if(a=="--mode")mode=need("--mode");
            else if(a=="--out")out=need("--out");
            else if(a=="--phase-out")phase_out=need("--phase-out");
            else if(a=="--trace-dir")trace_dir=need("--trace-dir");
            else if(a=="--run-id")run_id=need("--run-id");
            else if(a=="--timestamp-period-ns")timestamp_period=need("--timestamp-period-ns");
            else if(a=="--timestamp-valid-bits")timestamp_bits=need("--timestamp-valid-bits");
            else if(a=="--self-test")self_test=true;
            else throw std::runtime_error("unknown argument: "+a);
        }
        if(self_test){
            if(parse_u32_csv("1,2,3").size()!=3u)throw std::runtime_error("CSV parser self-test");
            std::cout<<"CORE0E_ARCLLM_CLI_SELF_TEST=PASS\n";
            return 0;
        }
        if(model.empty()||shader_dir.empty()||out.empty()||input.empty())
            throw std::runtime_error("required --model --shader-dir --tokens --out");
        if(forced.size()!=31u)throw std::runtime_error("CORE0E requires exactly 31 forced decode tokens");
        for(auto t:forced)if(t>=152064u)throw std::runtime_error("forced decode token out of vocabulary");
        if(profile>1u)throw std::runtime_error("--profile must be 0 or 1");
        if(mode!="CONTROL_UNINSTRUMENTED"&&mode!="COMBINED_PHASE_TRACE")
            throw std::runtime_error("invalid CORE0E mode");

        env_set("ARCLLM_CORE0D_FORCED_DECODE_IDS",csv(forced));
        env_clear("ARCLLM_CORE0E_PHASE_OUT");
        env_clear("ARCLLM_CORE0C_TRACE_DIR");
        env_clear("ARCLLM_CORE0C_RUN_ID");
        env_clear("ARCLLM_CORE0C_REQUEST_ID");
        env_clear("ARCLLM_CORE0C_TIMESTAMP_PERIOD_NS");
        env_clear("ARCLLM_CORE0C_TIMESTAMP_VALID_BITS");

        if(mode=="COMBINED_PHASE_TRACE"){
            if(phase_out.empty())throw std::runtime_error("COMBINED_PHASE_TRACE requires --phase-out");
            env_set("ARCLLM_CORE0E_PHASE_OUT",phase_out);
        }
        if(mode=="COMBINED_PHASE_TRACE"){
            if(trace_dir.empty()||run_id.empty()||timestamp_period.empty()||timestamp_bits.empty())
                throw std::runtime_error("COMBINED_PHASE_TRACE requires trace dir/run id/timestamp contract");
            env_set("ARCLLM_CORE0C_TRACE_DIR",trace_dir);
            env_set("ARCLLM_CORE0C_RUN_ID",run_id);
            env_set("ARCLLM_CORE0C_REQUEST_ID",run_id+"-request");
            env_set("ARCLLM_CORE0C_TIMESTAMP_PERIOD_NS",timestamp_period);
            env_set("ARCLLM_CORE0C_TIMESTAMP_VALID_BITS",timestamp_bits);
        }

        arcllm::v1::runtime::RunRequest req;
        req.model_path=model;
        req.shader_dir=shader_dir;
        req.input_token_ids=input;
        req.max_new_tokens=32u;
        req.evidence_profile=profile==0u?arcllm::v1::runtime::EvidenceProfile::PROFILE_0:arcllm::v1::runtime::EvidenceProfile::PROFILE_1;
        req.request_within_validated_domain=true;
        auto result=arcllm::v1::runtime::generate(req);

        std::ofstream o(out,std::ios::binary|std::ios::trunc);
        if(!o)throw std::runtime_error("cannot write --out");
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.core0e.arc_teacher_forced.v0.1\",\n";
        o<<"  \"system\":\"ArcLLM\",\n";
        o<<"  \"mode\":\""<<mode<<"\",\n";
        o<<"  \"teacher_forced\":true,\n";
        o<<"  \"success\":true,\n";
        o<<"  \"predicted_token_ids\":[";
        for(size_t i=0;i<result.generated_token_ids.size();++i){if(i)o<<",";o<<result.generated_token_ids[i];}
        o<<"],\n";
        o<<"  \"forced_decode_input_ids\":[";
        for(size_t i=0;i<forced.size();++i){if(i)o<<",";o<<forced[i];}
        o<<"],\n";
        o<<"  \"stats\":{";
        o<<"\"prefill_dispatches\":"<<result.stats.prefill_dispatches;
        o<<",\"prefill_submits\":"<<result.stats.prefill_submits;
        o<<",\"decode_dispatches_per_step\":"<<result.stats.decode_dispatches_per_step;
        o<<",\"decode_submits_per_step\":"<<result.stats.decode_submits_per_step;
        o<<",\"decode_steps\":"<<result.stats.decode_steps;
        o<<",\"route_a_steps\":"<<result.stats.route_a_steps;
        o<<",\"route_b_steps\":"<<result.stats.route_b_steps;
        o<<",\"acquire_events\":"<<result.stats.acquire_events;
        o<<",\"evict_events\":"<<result.stats.evict_events;
        o<<",\"b_allocations\":"<<result.stats.b_allocations;
        o<<",\"b_materializations\":"<<result.stats.b_materializations;
        o<<",\"b_validations\":"<<result.stats.b_validations;
        o<<",\"b_releases\":"<<result.stats.b_releases;
        o<<",\"p1_calls\":"<<result.stats.p1_calls;
        o<<",\"p3_calls\":"<<result.stats.p3_calls;
        o<<",\"p0_calls\":"<<result.stats.p0_calls;
        o<<",\"finite\":"<<(result.stats.finite?"true":"false")<<"}\n";
        o<<"}\n";
        return result.stats.finite&&result.generated_token_ids.size()==32u?0:3;
    }catch(const std::exception&e){
        std::cerr<<"CORE0E ArcLLM adapter error: "<<e.what()<<"\n";
        if(!out.empty()){
            std::ofstream o(out,std::ios::binary|std::ios::trunc);
            if(o)o<<"{\n  \"schema\":\"arcllm.core0e.arc_teacher_forced.v0.1\",\n  \"success\":false,\n  \"error\":\""<<e.what()<<"\"\n}\n";
        }
        return 2;
    }
}
