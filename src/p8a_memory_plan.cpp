
#include "gguf.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr uint32_t F32=0, Q4_K=12, Q6_K=14;
constexpr uint64_t QK_K=256, Q4_BYTES=144, Q6_BYTES=210;
constexpr uint64_t EXPECTED_FILE_BYTES=4683073536ull;
constexpr uint64_t ARENA_CAP=268435456ull;
constexpr uint64_t MAX_CTX=4096ull;
constexpr uint64_t PREFILL=512ull;
constexpr uint64_t BUDGET=18522046464ull;
constexpr uint64_t SAFETY=2147483648ull;
constexpr uint64_t USABLE=BUDGET-SAFETY;

uint64_t checked_add(uint64_t a,uint64_t b){if(b>(std::numeric_limits<uint64_t>::max)()-a)throw std::runtime_error("uint64 add overflow");return a+b;}
uint64_t checked_mul(uint64_t a,uint64_t b){if(a&&b>(std::numeric_limits<uint64_t>::max)()/a)throw std::runtime_error("uint64 mul overflow");return a*b;}
uint64_t elements(const GgufTensorInfo&t){uint64_t n=1;for(auto d:t.dims)n=checked_mul(n,d);return n;}
uint64_t tensor_bytes(const GgufTensorInfo&t){
 uint64_t n=elements(t);
 if(t.ggml_type==F32)return checked_mul(n,4);
 if(t.ggml_type==Q4_K){if(n%QK_K)throw std::runtime_error("Q4_K tensor not block divisible: "+t.name);return checked_mul(n/QK_K,Q4_BYTES);}
 if(t.ggml_type==Q6_K){if(n%QK_K)throw std::runtime_error("Q6_K tensor not block divisible: "+t.name);return checked_mul(n/QK_K,Q6_BYTES);}
 throw std::runtime_error("unsupported tensor type "+std::to_string(t.ggml_type)+": "+t.name);
}
std::string type_name(uint32_t t){if(t==F32)return"F32";if(t==Q4_K)return"Q4_K";if(t==Q6_K)return"Q6_K";return"TYPE_"+std::to_string(t);}
uint64_t must_u64(const GgufInfo&g,const std::string&k){auto it=g.scalars.find(k);if(it==g.scalars.end())throw std::runtime_error("missing GGUF metadata: "+k);return std::stoull(it->second);}
std::string must_str(const GgufInfo&g,const std::string&k){auto it=g.scalars.find(k);if(it==g.scalars.end())throw std::runtime_error("missing GGUF metadata: "+k);return it->second;}
const GgufTensorInfo* find_tensor(const GgufInfo&g,const std::string&n){for(const auto&t:g.tensors)if(t.name==n)return&t;return nullptr;}
std::string esc(const std::string&s){std::string o;for(char c:s){if(c=='\\'||c=='"'){o+='\\';o+=c;}else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else o+=c;}return o;}
struct Span{const GgufTensorInfo*t;uint64_t bytes,end;};
struct Arena{uint64_t start,end;};
}

int main(int argc,char**argv){
 std::string model,out="p8a_memory_plan.json";
 try{
  for(int i=1;i<argc;++i){
   std::string a=argv[i];
   if(a=="--model"&&i+1<argc)model=argv[++i];
   else if(a=="--out"&&i+1<argc)out=argv[++i];
   else throw std::runtime_error("usage: arcllm_p8a --model <gguf> [--out <json>]");
  }
  if(model.empty())throw std::runtime_error("--model required");
  uint64_t file_bytes=std::filesystem::file_size(std::filesystem::u8path(model));
  GgufInfo g=GgufReader(model).read();

  std::string arch=must_str(g,"general.architecture");
  uint64_t layers=must_u64(g,"qwen2.block_count");
  uint64_t hidden=must_u64(g,"qwen2.embedding_length");
  uint64_t qheads=must_u64(g,"qwen2.attention.head_count");
  uint64_t kvheads=must_u64(g,"qwen2.attention.head_count_kv");
  uint64_t ffn=must_u64(g,"qwen2.feed_forward_length");
  uint64_t model_ctx=must_u64(g,"qwen2.context_length");
  if(qheads==0||kvheads==0||hidden%qheads||qheads%kvheads)throw std::runtime_error("invalid head metadata");
  uint64_t head_dim=hidden/qheads,kv_dim=checked_mul(kvheads,head_dim);
  const auto*emb=find_tensor(g,"token_embd.weight");
  if(!emb||emb->dims.size()!=2||emb->dims[0]!=hidden)throw std::runtime_error("token_embd.weight shape mismatch");
  uint64_t vocab=emb->dims[1];

  std::vector<Span> spans;spans.reserve(g.tensors.size());
  std::map<uint32_t,uint64_t> type_counts,type_bytes;
  std::vector<std::string> unsupported,oversize;
  uint64_t tensor_total=0,payload_span=0;
  bool tensor_sizes_ok=true;
  for(const auto&t:g.tensors){
   try{
    uint64_t b=tensor_bytes(t),e=checked_add(t.offset,b);
    spans.push_back({&t,b,e});
    type_counts[t.ggml_type]++;type_bytes[t.ggml_type]=checked_add(type_bytes[t.ggml_type],b);
    tensor_total=checked_add(tensor_total,b);payload_span=(std::max)(payload_span,e);
    if(b>ARENA_CAP)oversize.push_back(t.name);
   }catch(const std::exception&){
    tensor_sizes_ok=false;unsupported.push_back(t.name+"@type="+std::to_string(t.ggml_type));
   }
  }
  std::sort(spans.begin(),spans.end(),[](const Span&a,const Span&b){return a.t->offset<b.t->offset;});
  bool no_overlap=tensor_sizes_ok;uint64_t prev=0;
  for(const auto&s:spans){if(s.t->offset<prev){no_overlap=false;break;}prev=s.end;}

  std::vector<Arena> arenas;bool arena_ok=tensor_sizes_ok&&no_overlap&&oversize.empty();
  if(arena_ok){
   uint64_t start=0;
   for(const auto&s:spans){
    if(s.end-start>ARENA_CAP&&s.t->offset>start){arenas.push_back({start,s.t->offset});start=s.t->offset;}
    if(s.end-start>ARENA_CAP){arena_ok=false;oversize.push_back(s.t->name);break;}
   }
   if(arena_ok&&payload_span>start)arenas.push_back({start,payload_span});
   for(const auto&a:arenas)if(a.end<=a.start||a.end-a.start>ARENA_CAP)arena_ok=false;
   if(arena_ok){
    for(const auto&s:spans){
     bool found=false;
     for(const auto&a:arenas)if(s.t->offset>=a.start&&s.end<=a.end){found=true;break;}
     if(!found){arena_ok=false;break;}
    }
   }
  }

  uint64_t kv_elems=checked_mul(checked_mul(checked_mul(layers,MAX_CTX),kv_dim),2);
  uint64_t kv_bytes=checked_mul(kv_elems,4);
  uint64_t hidden_terms=checked_mul(11,hidden),kv_terms=checked_mul(3,kv_dim),ffn_terms=checked_mul(3,ffn);
  uint64_t pp_floats=checked_mul(PREFILL,checked_add(checked_add(hidden_terms,kv_terms),ffn_terms));
  uint64_t work_bytes=checked_add(checked_mul(checked_add(pp_floats,vocab),4),checked_add(checked_mul(PREFILL,4),4));
  uint64_t total_planned=checked_add(checked_add(payload_span,kv_bytes),work_bytes);

  bool size_pass=file_bytes==EXPECTED_FILE_BYTES;
  bool arch_pass=arch=="qwen2";
  bool ctx_pass=model_ctx>=MAX_CTX;
  bool types_pass=unsupported.empty();
  bool capacity_pass=total_planned<=USABLE;
  bool pass=size_pass&&arch_pass&&ctx_pass&&types_pass&&no_overlap&&arena_ok&&oversize.empty()&&capacity_pass;

  std::ofstream o(out,std::ios::binary);if(!o)throw std::runtime_error("cannot open output JSON");
  o<<"{\n";
  o<<"  \"schema\":\"arcllm.p8a.memory_plan.v1\",\n";
  o<<"  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";
  o<<"  \"file\":{\"path\":\""<<esc(model)<<"\",\"size_bytes\":"<<file_bytes<<",\"expected_size_bytes\":"<<EXPECTED_FILE_BYTES<<",\"size_pass\":"<<(size_pass?"true":"false")<<"},\n";
  o<<"  \"model\":{\"architecture\":\""<<esc(arch)<<"\",\"layers\":"<<layers<<",\"hidden\":"<<hidden<<",\"q_heads\":"<<qheads<<",\"kv_heads\":"<<kvheads<<",\"head_dim\":"<<head_dim<<",\"kv_dim\":"<<kv_dim<<",\"ffn\":"<<ffn<<",\"vocab\":"<<vocab<<",\"model_context\":"<<model_ctx<<",\"planned_context\":"<<MAX_CTX<<",\"prefill_chunk\":"<<PREFILL<<"},\n";
  o<<"  \"gguf\":{\"version\":"<<g.version<<",\"tensor_count\":"<<g.tensor_count<<",\"metadata_count\":"<<g.metadata_count<<",\"alignment\":"<<g.alignment<<",\"data_offset\":"<<g.data_offset<<",\"tensor_bytes_total\":"<<tensor_total<<",\"payload_span_bytes\":"<<payload_span<<",\"no_overlap\":"<<(no_overlap?"true":"false")<<"},\n";
  o<<"  \"quant_mix\":[";
  bool first=true;
  for(const auto&kv:type_counts){if(!first)o<<",";first=false;o<<"{\"ggml_type\":"<<kv.first<<",\"name\":\""<<type_name(kv.first)<<"\",\"count\":"<<kv.second<<",\"bytes\":"<<type_bytes[kv.first]<<"}";}
  o<<"],\n";
  o<<"  \"unsupported_tensors\":[";
  for(size_t i=0;i<unsupported.size();++i){if(i)o<<",";o<<"\""<<esc(unsupported[i])<<"\"";}
  o<<"],\n";
  o<<"  \"oversize_tensors\":[";
  for(size_t i=0;i<oversize.size();++i){if(i)o<<",";o<<"\""<<esc(oversize[i])<<"\"";}
  o<<"],\n";
  o<<"  \"arena_plan\":{\"cap_bytes\":"<<ARENA_CAP<<",\"count\":"<<arenas.size()<<",\"pass\":"<<(arena_ok?"true":"false")<<",\"arenas\":[";
  for(size_t i=0;i<arenas.size();++i){if(i)o<<",";o<<"{\"index\":"<<i<<",\"start\":"<<arenas[i].start<<",\"end\":"<<arenas[i].end<<",\"bytes\":"<<(arenas[i].end-arenas[i].start)<<"}";}
  o<<"]},\n";
  o<<"  \"memory\":{\"weight_payload_span_bytes\":"<<payload_span<<",\"kv_fp32_bytes\":"<<kv_bytes<<",\"working_buffer_bytes\":"<<work_bytes<<",\"total_planned_bytes\":"<<total_planned<<",\"observed_budget_bytes\":"<<BUDGET<<",\"safety_margin_bytes\":"<<SAFETY<<",\"usable_budget_bytes\":"<<USABLE<<",\"headroom_bytes\":"<<(total_planned<=USABLE?USABLE-total_planned:0)<<",\"capacity_pass\":"<<(capacity_pass?"true":"false")<<"},\n";
  o<<"  \"gate\":{\"size_pass\":"<<(size_pass?"true":"false")<<",\"architecture_pass\":"<<(arch_pass?"true":"false")<<",\"context_pass\":"<<(ctx_pass?"true":"false")<<",\"supported_types_pass\":"<<(types_pass?"true":"false")<<",\"no_overlap_pass\":"<<(no_overlap?"true":"false")<<",\"arena_pack_pass\":"<<(arena_ok?"true":"false")<<",\"no_oversize_tensor_pass\":"<<(oversize.empty()?"true":"false")<<",\"capacity_pass\":"<<(capacity_pass?"true":"false")<<",\"p8a_pass\":"<<(pass?"true":"false")<<"}\n";
  o<<"}\n";o.close();

  std::cout<<"ArcLLM P8-A memory-plan-only bring-up\n";
  std::cout<<"model layers="<<layers<<" hidden="<<hidden<<" ffn="<<ffn<<" vocab="<<vocab<<" kv_dim="<<kv_dim<<" tensors="<<g.tensor_count<<"\n";
  std::cout<<"weights_span="<<payload_span<<" kv="<<kv_bytes<<" work="<<work_bytes<<" total="<<total_planned<<" usable="<<USABLE<<"\n";
  std::cout<<"arenas="<<arenas.size()<<" oversize_tensors="<<oversize.size()<<" unsupported="<<unsupported.size()<<"\n";
  for(const auto&n:oversize)std::cout<<"  oversize: "<<n<<"\n";
  std::cout<<"P8-A "<<(pass?"PASS":"FAIL")<<"\n";
  return pass?0:20;
 }catch(const std::exception&e){
  std::ofstream o(out,std::ios::binary);
  if(o)o<<"{\n  \"schema\":\"arcllm.p8a.memory_plan.v1\",\n  \"status\":\"ERROR\",\n  \"error\":\""<<esc(e.what())<<"\"\n}\n";
  std::cerr<<e.what()<<"\n";return 2;
 }
}
