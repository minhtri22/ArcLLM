#include "../include/arcllm/v1/runtime.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint32_t> parse_tokens(const std::string& csv){
    std::vector<std::uint32_t> out;
    std::stringstream ss(csv);
    std::string item;
    while(std::getline(ss,item,',')){
        if(item.empty()) throw std::runtime_error("empty token in --tokens");
        const unsigned long long v=std::stoull(item);
        if(v>0xffffffffull) throw std::runtime_error("token exceeds uint32");
        out.push_back(static_cast<std::uint32_t>(v));
    }
    if(out.empty()) throw std::runtime_error("--tokens produced no tokens");
    return out;
}
void write_json(std::ostream& o,const arcllm::v1::runtime::RunResult& r){
    const auto&s=r.stats;
    o<<"{\n";
    o<<"  \"schema\":\"arcllm.v1.runtime.result.v0.1\",\n";
    o<<"  \"generated_token_ids\":[";
    for(std::size_t i=0;i<r.generated_token_ids.size();++i){
        if(i)o<<",";
        o<<r.generated_token_ids[i];
    }
    o<<"],\n";
    o<<"  \"stats\":{"
     <<"\"prefill_dispatches\":"<<s.prefill_dispatches
     <<",\"prefill_submits\":"<<s.prefill_submits
     <<",\"decode_dispatches_per_step\":"<<s.decode_dispatches_per_step
     <<",\"decode_submits_per_step\":"<<s.decode_submits_per_step
     <<",\"decode_steps\":"<<s.decode_steps
     <<",\"route_a_steps\":"<<s.route_a_steps
     <<",\"route_b_steps\":"<<s.route_b_steps
     <<",\"acquire_events\":"<<s.acquire_events
     <<",\"evict_events\":"<<s.evict_events
     <<",\"b_allocations\":"<<s.b_allocations
     <<",\"b_materializations\":"<<s.b_materializations
     <<",\"b_validations\":"<<s.b_validations
     <<",\"b_releases\":"<<s.b_releases
     <<",\"p1_calls\":"<<s.p1_calls
     <<",\"p3_calls\":"<<s.p3_calls
     <<",\"p0_calls\":"<<s.p0_calls
     <<",\"finite\":"<<(s.finite?"true":"false")<<"}\n";
    o<<"}\n";
}
}

int main(int argc,char**argv){
    try{
        arcllm::v1::runtime::RunRequest req;
        std::string token_csv,out_path;
        bool have_profile=false,have_scope=false;
        for(int i=1;i<argc;++i){
            const std::string a=argv[i];
            auto need=[&](const char*flag){
                if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+flag);
                return std::string(argv[++i]);
            };
            if(a=="--model")req.model_path=need("--model");
            else if(a=="--shader-dir")req.shader_dir=need("--shader-dir");
            else if(a=="--sidecar")req.sidecar_path=need("--sidecar");
            else if(a=="--tokens")token_csv=need("--tokens");
            else if(a=="--max-new")req.max_new_tokens=static_cast<std::uint32_t>(std::stoul(need("--max-new")));
            else if(a=="--profile"){
                const auto p=need("--profile");
                if(p=="0")req.evidence_profile=arcllm::v1::runtime::EvidenceProfile::PROFILE_0;
                else if(p=="1")req.evidence_profile=arcllm::v1::runtime::EvidenceProfile::PROFILE_1;
                else throw std::runtime_error("--profile must be 0 or 1");
                have_profile=true;
            }else if(a=="--within-validated-domain"){
                const auto v=need("--within-validated-domain");
                if(v=="1")req.request_within_validated_domain=true;
                else if(v=="0")req.request_within_validated_domain=false;
                else throw std::runtime_error("--within-validated-domain must be 0 or 1");
                have_scope=true;
            }else if(a=="--out")out_path=need("--out");
            else throw std::runtime_error("unknown argument: "+a);
        }
        if(req.model_path.empty()||req.shader_dir.empty()||token_csv.empty()||!have_profile||!have_scope)
            throw std::runtime_error("required: --model --shader-dir --tokens --max-new --profile --within-validated-domain");
        req.input_token_ids=parse_tokens(token_csv);
        const auto result=arcllm::v1::runtime::generate(req);
        if(out_path.empty())write_json(std::cout,result);
        else{
            std::ofstream o(out_path,std::ios::binary|std::ios::trunc);
            if(!o)throw std::runtime_error("cannot open --out path");
            write_json(o,result);
        }
        return 0;
    }catch(const std::exception&e){
        std::cerr<<"ArcLLM runtime error: "<<e.what()<<"\n";
        return 2;
    }
}
