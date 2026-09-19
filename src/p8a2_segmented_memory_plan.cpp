
#include "gguf.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr uint32_t F32=0, Q4_K=12, Q6_K=14;
constexpr uint64_t QK_K=256, Q4_BYTES=144, Q6_BYTES=210;
constexpr uint64_t EXPECTED_FILE_BYTES=4683074048ull;
constexpr uint64_t ARENA_CAP=268435456ull;
constexpr uint64_t MAX_CTX=4096ull;
constexpr uint64_t PREFILL=512ull;
constexpr uint64_t BUDGET=18522046464ull;
constexpr uint64_t SAFETY=2147483648ull;
constexpr uint64_t USABLE=BUDGET-SAFETY;

uint64_t checked_add(uint64_t a,uint64_t b){
    if(b>(std::numeric_limits<uint64_t>::max)()-a)throw std::runtime_error("uint64 add overflow");
    return a+b;
}
uint64_t checked_mul(uint64_t a,uint64_t b){
    if(a&&b>(std::numeric_limits<uint64_t>::max)()/a)throw std::runtime_error("uint64 mul overflow");
    return a*b;
}
uint64_t row_bytes(uint32_t type,uint64_t ne0){
    if(type==F32)return checked_mul(ne0,4);
    if(type==Q4_K){
        if(ne0%QK_K)throw std::runtime_error("Q4_K row not block divisible");
        return checked_mul(ne0/QK_K,Q4_BYTES);
    }
    if(type==Q6_K){
        if(ne0%QK_K)throw std::runtime_error("Q6_K row not block divisible");
        return checked_mul(ne0/QK_K,Q6_BYTES);
    }
    throw std::runtime_error("unsupported tensor type "+std::to_string(type));
}
uint64_t rows_of(const GgufTensorInfo&t){
    if(t.dims.empty())throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t rows=1;
    for(size_t i=1;i<t.dims.size();++i)rows=checked_mul(rows,t.dims[i]);
    return rows;
}
uint64_t tensor_bytes(const GgufTensorInfo&t){
    return checked_mul(row_bytes(t.ggml_type,t.dims.at(0)),rows_of(t));
}
std::string type_name(uint32_t t){
    if(t==F32)return"F32";
    if(t==Q4_K)return"Q4_K";
    if(t==Q6_K)return"Q6_K";
    return"TYPE_"+std::to_string(t);
}
uint64_t must_u64(const GgufInfo&g,const std::string&k){
    auto it=g.scalars.find(k);
    if(it==g.scalars.end())throw std::runtime_error("missing GGUF metadata: "+k);
    return std::stoull(it->second);
}
std::string must_str(const GgufInfo&g,const std::string&k){
    auto it=g.scalars.find(k);
    if(it==g.scalars.end())throw std::runtime_error("missing GGUF metadata: "+k);
    return it->second;
}
const GgufTensorInfo* find_tensor(const GgufInfo&g,const std::string&n){
    for(const auto&t:g.tensors)if(t.name==n)return&t;
    return nullptr;
}
std::string esc(const std::string&s){
    std::string o;
    for(char c:s){
        if(c=='\\'||c=='"'){o+='\\';o+=c;}
        else if(c=='\n')o+="\\n";
        else if(c=='\r')o+="\\r";
        else o+=c;
    }
    return o;
}
struct Span{
    const GgufTensorInfo*t=nullptr;
    uint64_t bytes=0;
    uint64_t end=0;
};
struct Piece{
    const GgufTensorInfo*t=nullptr;
    uint32_t segment_index=0;
    uint64_t row_start=0;
    uint64_t row_count=0;
    uint64_t row_bytes=0;
    uint64_t file_start=0;
    uint64_t bytes=0;
    uint64_t file_end=0;
    uint32_t arena=0xffffffffu;
    uint64_t arena_byte_base=0;
};
struct Arena{uint64_t start=0,end=0;};

bool tensor_rows_covered(const GgufTensorInfo&t,const std::vector<Piece>&pieces){
    uint64_t want=rows_of(t),next=0;
    uint32_t seg=0;
    bool any=false;
    for(const auto&p:pieces){
        if(p.t!=&t)continue;
        any=true;
        if(p.segment_index!=seg||p.row_start!=next||p.row_count==0)return false;
        next=checked_add(next,p.row_count);
        ++seg;
    }
    return any&&next==want;
}
void json_dims(std::ofstream&o,const std::vector<uint64_t>&d){
    o<<"[";
    for(size_t i=0;i<d.size();++i){if(i)o<<",";o<<d[i];}
    o<<"]";
}
}

int main(int argc,char**argv){
    std::string model,out="p8a2_segment_plan.json";
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            if(a=="--model"&&i+1<argc)model=argv[++i];
            else if(a=="--out"&&i+1<argc)out=argv[++i];
            else throw std::runtime_error("usage: arcllm_p8a2 --model <gguf> [--out <json>]");
        }
        if(model.empty())throw std::runtime_error("--model required");
        uint64_t file_bytes=std::filesystem::file_size(std::filesystem::u8path(model));
        GgufInfo g=GgufReader(model).read();

        const std::string arch=must_str(g,"general.architecture");
        const uint64_t layers=must_u64(g,"qwen2.block_count");
        const uint64_t hidden=must_u64(g,"qwen2.embedding_length");
        const uint64_t qheads=must_u64(g,"qwen2.attention.head_count");
        const uint64_t kvheads=must_u64(g,"qwen2.attention.head_count_kv");
        const uint64_t ffn=must_u64(g,"qwen2.feed_forward_length");
        const uint64_t model_ctx=must_u64(g,"qwen2.context_length");
        if(qheads==0||kvheads==0||hidden%qheads||qheads%kvheads)throw std::runtime_error("invalid head metadata");
        const uint64_t head_dim=hidden/qheads,kv_dim=checked_mul(kvheads,head_dim);
        const auto*emb=find_tensor(g,"token_embd.weight");
        const auto*outw=find_tensor(g,"output.weight");
        if(!emb||!outw)throw std::runtime_error("required vocab tensor missing");
        if(emb->dims.size()!=2||outw->dims.size()!=2)throw std::runtime_error("vocab tensors must be 2-D");
        if(emb->dims[0]!=hidden||outw->dims[0]!=hidden)throw std::runtime_error("vocab tensor hidden dimension mismatch");
        const uint64_t vocab=emb->dims[1];
        if(outw->dims[1]!=vocab)throw std::runtime_error("embedding/output vocab mismatch");

        std::vector<Span> spans;
        spans.reserve(g.tensors.size());
        std::map<uint32_t,uint64_t> type_counts,type_bytes;
        std::vector<std::string> unsupported,oversize_names;
        std::set<std::string> oversize_set;
        uint64_t tensor_total=0,payload_span=0;
        bool tensor_sizes_ok=true;
        for(const auto&t:g.tensors){
            try{
                uint64_t b=tensor_bytes(t),e=checked_add(t.offset,b);
                spans.push_back({&t,b,e});
                type_counts[t.ggml_type]++;
                type_bytes[t.ggml_type]=checked_add(type_bytes[t.ggml_type],b);
                tensor_total=checked_add(tensor_total,b);
                payload_span=(std::max)(payload_span,e);
                if(b>ARENA_CAP){oversize_names.push_back(t.name);oversize_set.insert(t.name);}
            }catch(const std::exception&){
                tensor_sizes_ok=false;
                unsupported.push_back(t.name+"@type="+std::to_string(t.ggml_type));
            }
        }
        std::sort(spans.begin(),spans.end(),[](const Span&a,const Span&b){return a.t->offset<b.t->offset;});
        bool no_overlap=tensor_sizes_ok;
        uint64_t prev_end=0;
        for(const auto&s:spans){
            if(s.t->offset<prev_end){no_overlap=false;break;}
            prev_end=s.end;
        }

        const std::set<std::string> expected_oversize={"token_embd.weight","output.weight"};
        const bool parent_obstruction_match=(oversize_set==expected_oversize);

        std::vector<Piece> pieces;
        bool segmentation_ok=tensor_sizes_ok&&no_overlap;
        for(const auto&s:spans){
            const auto&t=*s.t;
            const uint64_t rb=row_bytes(t.ggml_type,t.dims.at(0));
            const uint64_t rows=rows_of(t);
            if(s.bytes<=ARENA_CAP){
                pieces.push_back({&t,0,0,rows,rb,t.offset,s.bytes,s.end});
                continue;
            }
            if(t.dims.size()<2||rb==0){segmentation_ok=false;continue;}
            const uint64_t max_rows=ARENA_CAP/rb;
            if(max_rows==0){segmentation_ok=false;continue;}
            uint64_t row=0;
            uint32_t seg=0;
            while(row<rows){
                const uint64_t cnt=(std::min)(max_rows,rows-row);
                const uint64_t bytes=checked_mul(cnt,rb);
                const uint64_t start=checked_add(t.offset,checked_mul(row,rb));
                const uint64_t end=checked_add(start,bytes);
                if(bytes==0||bytes>ARENA_CAP||end>s.end){segmentation_ok=false;break;}
                pieces.push_back({&t,seg,row,cnt,rb,start,bytes,end});
                row=checked_add(row,cnt);
                ++seg;
            }
        }
        std::sort(pieces.begin(),pieces.end(),[](const Piece&a,const Piece&b){
            if(a.file_start!=b.file_start)return a.file_start<b.file_start;
            return a.segment_index<b.segment_index;
        });

        bool row_coverage_pass=segmentation_ok;
        for(const auto&name:oversize_names){
            const auto*t=find_tensor(g,name);
            if(!t||!tensor_rows_covered(*t,pieces)){row_coverage_pass=false;break;}
        }

        bool piece_order_pass=segmentation_ok&&no_overlap;
        uint64_t piece_prev=0,piece_bytes_total=0;
        for(const auto&p:pieces){
            if(p.file_start<piece_prev||p.file_end<p.file_start||p.bytes!=p.file_end-p.file_start||p.bytes>ARENA_CAP){
                piece_order_pass=false;break;
            }
            piece_prev=p.file_end;
            piece_bytes_total=checked_add(piece_bytes_total,p.bytes);
        }
        if(piece_bytes_total!=tensor_total)piece_order_pass=false;

        std::vector<Arena> arenas;
        bool arena_pack_pass=piece_order_pass&&!pieces.empty();
        if(arena_pack_pass){
            uint64_t start=0;
            for(const auto&p:pieces){
                if(p.file_end-start>ARENA_CAP){
                    if(p.file_start<=start){arena_pack_pass=false;break;}
                    arenas.push_back({start,p.file_start});
                    start=p.file_start;
                }
                if(p.file_end-start>ARENA_CAP){arena_pack_pass=false;break;}
            }
            if(arena_pack_pass&&payload_span>start)arenas.push_back({start,payload_span});
            for(const auto&a:arenas){
                if(a.end<=a.start||a.end-a.start>ARENA_CAP){arena_pack_pass=false;break;}
            }
        }

        bool contained_once_pass=arena_pack_pass;
        if(contained_once_pass){
            for(auto&p:pieces){
                uint32_t matches=0,which=0;
                uint64_t base=0;
                for(uint32_t ai=0;ai<uint32_t(arenas.size());++ai){
                    if(p.file_start>=arenas[ai].start&&p.file_end<=arenas[ai].end){
                        ++matches;which=ai;base=p.file_start-arenas[ai].start;
                    }
                }
                if(matches!=1){contained_once_pass=false;break;}
                p.arena=which;p.arena_byte_base=base;
                if(base>0xffffffffull){contained_once_pass=false;break;}
            }
        }

        auto segment_count=[&](const GgufTensorInfo*t){
            uint32_t n=0;for(const auto&p:pieces)if(p.t==t)++n;return n;
        };
        const bool embedding_addressable=
            emb->dims.size()==2&&emb->dims[0]==hidden&&emb->dims[1]==vocab&&
            (emb->ggml_type==Q4_K||emb->ggml_type==Q6_K)&&
            segment_count(emb)>=2&&tensor_rows_covered(*emb,pieces);
        const bool output_addressable=
            outw->dims.size()==2&&outw->dims[0]==hidden&&outw->dims[1]==vocab&&
            (outw->ggml_type==Q4_K||outw->ggml_type==Q6_K)&&
            segment_count(outw)>=2&&tensor_rows_covered(*outw,pieces);
        const bool addressability_pass=embedding_addressable&&output_addressable;

        const uint64_t kv_elems=checked_mul(checked_mul(checked_mul(layers,MAX_CTX),kv_dim),2);
        const uint64_t kv_bytes=checked_mul(kv_elems,4);
        const uint64_t hidden_terms=checked_mul(11,hidden),kv_terms=checked_mul(3,kv_dim),ffn_terms=checked_mul(3,ffn);
        const uint64_t pp_floats=checked_mul(PREFILL,checked_add(checked_add(hidden_terms,kv_terms),ffn_terms));
        const uint64_t work_bytes=checked_add(checked_mul(checked_add(pp_floats,vocab),4),checked_add(checked_mul(PREFILL,4),4));
        const uint64_t total_planned=checked_add(checked_add(payload_span,kv_bytes),work_bytes);

        const bool size_pass=file_bytes==EXPECTED_FILE_BYTES;
        const bool arch_pass=arch=="qwen2";
        const bool ctx_pass=model_ctx>=MAX_CTX;
        const bool types_pass=unsupported.empty();
        const bool capacity_pass=total_planned<=USABLE;
        const bool pass=size_pass&&arch_pass&&ctx_pass&&types_pass&&no_overlap&&
                        parent_obstruction_match&&segmentation_ok&&row_coverage_pass&&
                        piece_order_pass&&arena_pack_pass&&contained_once_pass&&
                        addressability_pass&&capacity_pass;

        std::ofstream o(out,std::ios::binary);
        if(!o)throw std::runtime_error("cannot open output JSON");
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.p8a2.segmented_memory_plan.v1\",\n";
        o<<"  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";
        o<<"  \"file\":{\"path\":\""<<esc(model)<<"\",\"size_bytes\":"<<file_bytes<<",\"expected_size_bytes\":"<<EXPECTED_FILE_BYTES<<",\"size_pass\":"<<(size_pass?"true":"false")<<"},\n";
        o<<"  \"model\":{\"architecture\":\""<<esc(arch)<<"\",\"layers\":"<<layers<<",\"hidden\":"<<hidden<<",\"q_heads\":"<<qheads<<",\"kv_heads\":"<<kvheads<<",\"head_dim\":"<<head_dim<<",\"kv_dim\":"<<kv_dim<<",\"ffn\":"<<ffn<<",\"vocab\":"<<vocab<<",\"model_context\":"<<model_ctx<<",\"planned_context\":"<<MAX_CTX<<",\"prefill_chunk\":"<<PREFILL<<"},\n";
        o<<"  \"parent_obstruction\":{\"expected\":[\"output.weight\",\"token_embd.weight\"],\"observed\":[";
        for(size_t i=0;i<oversize_names.size();++i){if(i)o<<",";o<<"\""<<esc(oversize_names[i])<<"\"";}
        o<<"],\"match\":"<<(parent_obstruction_match?"true":"false")<<"},\n";
        o<<"  \"gguf\":{\"version\":"<<g.version<<",\"tensor_count\":"<<g.tensor_count<<",\"metadata_count\":"<<g.metadata_count<<",\"alignment\":"<<g.alignment<<",\"data_offset\":"<<g.data_offset<<",\"tensor_bytes_total\":"<<tensor_total<<",\"payload_span_bytes\":"<<payload_span<<",\"no_overlap\":"<<(no_overlap?"true":"false")<<"},\n";
        o<<"  \"quant_mix\":[";
        bool first=true;
        for(const auto&kv:type_counts){if(!first)o<<",";first=false;o<<"{\"ggml_type\":"<<kv.first<<",\"name\":\""<<type_name(kv.first)<<"\",\"count\":"<<kv.second<<",\"bytes\":"<<type_bytes[kv.first]<<"}";}
        o<<"],\n";
        o<<"  \"segmented_tensors\":[";
        bool first_tensor=true;
        for(const auto&name:oversize_names){
            const auto*t=find_tensor(g,name);
            if(!t)continue;
            if(!first_tensor)o<<",";
            first_tensor=false;
            const uint64_t rb=row_bytes(t->ggml_type,t->dims[0]),rows=rows_of(*t),tb=tensor_bytes(*t);
            o<<"{\"name\":\""<<esc(t->name)<<"\",\"ggml_type\":"<<t->ggml_type<<",\"type_name\":\""<<type_name(t->ggml_type)<<"\",\"dims\":";
            json_dims(o,t->dims);
            o<<",\"row_bytes\":"<<rb<<",\"rows\":"<<rows<<",\"tensor_bytes\":"<<tb<<",\"segment_count\":"<<segment_count(t)<<",\"segments\":[";
            bool fs=true;
            for(const auto&p:pieces)if(p.t==t){
                if(!fs)o<<",";fs=false;
                o<<"{\"index\":"<<p.segment_index<<",\"row_start\":"<<p.row_start<<",\"row_count\":"<<p.row_count<<",\"file_start\":"<<p.file_start<<",\"file_end\":"<<p.file_end<<",\"bytes\":"<<p.bytes<<",\"arena\":"<<p.arena<<",\"arena_byte_base\":"<<p.arena_byte_base<<"}";
            }
            o<<"]}";
        }
        o<<"],\n";
        o<<"  \"addressability\":{\"embedding_global_row_to_segment_local_row\":"<<(embedding_addressable?"true":"false")<<",\"lm_head_global_row_to_segment_local_row\":"<<(output_addressable?"true":"false")<<",\"pass\":"<<(addressability_pass?"true":"false")<<"},\n";
        o<<"  \"arena_plan\":{\"cap_bytes\":"<<ARENA_CAP<<",\"count\":"<<arenas.size()<<",\"piece_count\":"<<pieces.size()<<",\"piece_bytes_total\":"<<piece_bytes_total<<",\"row_coverage_pass\":"<<(row_coverage_pass?"true":"false")<<",\"contained_once_pass\":"<<(contained_once_pass?"true":"false")<<",\"pass\":"<<(arena_pack_pass&&contained_once_pass?"true":"false")<<",\"arenas\":[";
        for(size_t i=0;i<arenas.size();++i){if(i)o<<",";o<<"{\"index\":"<<i<<",\"start\":"<<arenas[i].start<<",\"end\":"<<arenas[i].end<<",\"bytes\":"<<(arenas[i].end-arenas[i].start)<<"}";}
        o<<"]},\n";
        o<<"  \"memory\":{\"weight_payload_span_bytes\":"<<payload_span<<",\"kv_fp32_bytes\":"<<kv_bytes<<",\"working_buffer_bytes\":"<<work_bytes<<",\"total_planned_bytes\":"<<total_planned<<",\"observed_budget_bytes\":"<<BUDGET<<",\"safety_margin_bytes\":"<<SAFETY<<",\"usable_budget_bytes\":"<<USABLE<<",\"headroom_bytes\":"<<(total_planned<=USABLE?USABLE-total_planned:0)<<",\"capacity_pass\":"<<(capacity_pass?"true":"false")<<"},\n";
        o<<"  \"gate\":{\"size_pass\":"<<(size_pass?"true":"false")<<",\"architecture_pass\":"<<(arch_pass?"true":"false")<<",\"context_pass\":"<<(ctx_pass?"true":"false")<<",\"supported_types_pass\":"<<(types_pass?"true":"false")<<",\"no_overlap_pass\":"<<(no_overlap?"true":"false")<<",\"parent_obstruction_match\":"<<(parent_obstruction_match?"true":"false")<<",\"segmentation_pass\":"<<(segmentation_ok?"true":"false")<<",\"row_coverage_pass\":"<<(row_coverage_pass?"true":"false")<<",\"piece_order_pass\":"<<(piece_order_pass?"true":"false")<<",\"arena_pack_pass\":"<<(arena_pack_pass?"true":"false")<<",\"contained_once_pass\":"<<(contained_once_pass?"true":"false")<<",\"addressability_pass\":"<<(addressability_pass?"true":"false")<<",\"capacity_pass\":"<<(capacity_pass?"true":"false")<<",\"p8a2_pass\":"<<(pass?"true":"false")<<"}\n";
        o<<"}\n";
        o.close();

        std::cout<<"ArcLLM P8-A2 segmented large-tensor plan\n";
        std::cout<<"model layers="<<layers<<" hidden="<<hidden<<" ffn="<<ffn<<" vocab="<<vocab<<" tensors="<<g.tensor_count<<"\n";
        std::cout<<"parent oversize tensors="<<oversize_names.size()<<" match="<<parent_obstruction_match<<"\n";
        for(const auto&name:oversize_names){
            const auto*t=find_tensor(g,name);
            if(!t)continue;
            std::cout<<"  "<<name<<" type="<<type_name(t->ggml_type)<<" bytes="<<tensor_bytes(*t)<<" row_bytes="<<row_bytes(t->ggml_type,t->dims[0])<<" segments="<<segment_count(t)<<"\n";
        }
        std::cout<<"arenas="<<arenas.size()<<" pieces="<<pieces.size()<<" addressability="<<addressability_pass<<"\n";
        std::cout<<"total="<<total_planned<<" usable="<<USABLE<<" headroom="<<(total_planned<=USABLE?USABLE-total_planned:0)<<"\n";
        std::cout<<"P8-A2 "<<(pass?"PASS":"FAIL")<<"\n";
        return pass?0:20;
    }catch(const std::exception&e){
        std::ofstream o(out,std::ios::binary);
        if(o)o<<"{\n  \"schema\":\"arcllm.p8a2.segmented_memory_plan.v1\",\n  \"status\":\"ERROR\",\n  \"error\":\""<<esc(e.what())<<"\"\n}\n";
        std::cerr<<e.what()<<"\n";
        return 2;
    }
}
