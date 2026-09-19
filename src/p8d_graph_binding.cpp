#define main p8c_main_disabled
#include "p8c_segmented_access_correctness.cpp"
#undef main

#include <map>
#include <set>

struct P8DArenaPlan { uint64_t start=0,end=0; };
struct P8DBindingSlice {
    uint32_t arena=0;
    uint64_t arena_byte_base=0;
    uint64_t source_start=0;
    uint64_t bytes=0;
    uint64_t row_start=0;
    uint64_t row_count=0;
};
struct TensorBindingDescriptor {
    std::string name;
    uint32_t ggml_type=0;
    uint64_t row_bytes=0;
    uint64_t rows=0;
    uint64_t tensor_bytes=0;
    std::vector<P8DBindingSlice> slices;
};

static constexpr uint64_t P8D_ARENA_CAP=268435456ull;

static std::string p8d_read_text(const std::string& path) {
    std::ifstream f(path,std::ios::binary);
    if(!f) throw std::runtime_error("cannot open required parent input: "+path);
    std::ostringstream ss; ss<<f.rdbuf(); return ss.str();
}
static uint64_t p8d_rows_of(const GgufTensorInfo& t) {
    if(t.dims.empty()) throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t r=1; for(size_t i=1;i<t.dims.size();++i) r*=t.dims[i]; return r;
}
static uint64_t p8d_row_bytes(const GgufTensorInfo& t) {
    if(t.dims.empty()) throw std::runtime_error("tensor has no dims: "+t.name);
    const uint64_t n=t.dims[0];
    if(t.ggml_type==0u) return n*4ull;
    if(t.ggml_type==12u) {
        if(n%256ull) throw std::runtime_error("Q4_K row width invalid: "+t.name);
        return (n/256ull)*144ull;
    }
    if(t.ggml_type==14u) {
        if(n%256ull) throw std::runtime_error("Q6_K row width invalid: "+t.name);
        return (n/256ull)*210ull;
    }
    throw std::runtime_error("unsupported tensor type in P8-D: "+t.name);
}
static std::vector<P8DArenaPlan> p8d_expected_arenas() {
    return {
        {0ull,268434432ull},{268434432ull,511383552ull},{511383552ull,753913856ull},
        {753913856ull,1016184832ull},{1016184832ull,1280919552ull},{1280919552ull,1538461696ull},
        {1538461696ull,1800732672ull},{1800732672ull,2042789888ull},{2042789888ull,2267342848ull},
        {2267342848ull,2531604480ull},{2531604480ull,2755108864ull},{2755108864ull,3018350592ull},
        {3018350592ull,3260407808ull},{3260407808ull,3484487680ull},{3484487680ull,3727486976ull},
        {3727486976ull,3987521536ull},{3987521536ull,4230051840ull},{4230051840ull,4498485600ull},
        {4498485600ull,4677120000ull}
    };
}
static std::vector<std::pair<uint64_t,uint64_t>> p8d_tensor_pieces(const GgufTensorInfo& t) {
    const uint64_t rb=p8d_row_bytes(t), rows=p8d_rows_of(t), bytes=rb*rows;
    std::vector<std::pair<uint64_t,uint64_t>> out;
    if(bytes<=P8D_ARENA_CAP) {
        out.push_back({t.offset,t.offset+bytes});
        return out;
    }
    const uint64_t max_rows=P8D_ARENA_CAP/rb;
    if(max_rows==0) throw std::runtime_error("P8-D row exceeds arena cap: "+t.name);
    uint64_t row=0;
    while(row<rows) {
        const uint64_t cnt=(std::min)(max_rows,rows-row);
        const uint64_t s=t.offset+row*rb, e=s+cnt*rb;
        out.push_back({s,e});
        row+=cnt;
    }
    return out;
}
static std::vector<P8DArenaPlan> p8d_recompute_arenas(const GgufInfo& g) {
    struct Piece { uint64_t start,end; };
    std::vector<const GgufTensorInfo*> ts;
    for(const auto& t:g.tensors) ts.push_back(&t);
    std::sort(ts.begin(),ts.end(),[](auto* a,auto* b){return a->offset<b->offset;});
    std::vector<Piece> pieces;
    uint64_t payload=0;
    for(auto* t:ts) {
        for(auto p:p8d_tensor_pieces(*t)) pieces.push_back({p.first,p.second});
        payload=(std::max)(payload,t->offset+tensor_nbytes(*t));
    }
    std::sort(pieces.begin(),pieces.end(),[](const Piece& a,const Piece& b){return a.start<b.start;});
    std::vector<P8DArenaPlan> out;
    uint64_t start=0;
    for(const auto& p:pieces) {
        if(p.end-start>P8D_ARENA_CAP) {
            if(p.start<=start) throw std::runtime_error("P8-D cannot pack piece");
            out.push_back({start,p.start});
            start=p.start;
        }
        if(p.end-start>P8D_ARENA_CAP) throw std::runtime_error("P8-D piece exceeds arena cap");
    }
    if(payload>start) out.push_back({start,payload});
    return out;
}
static bool p8d_arenas_equal(const std::vector<P8DArenaPlan>& a,const std::vector<P8DArenaPlan>& b) {
    if(a.size()!=b.size()) return false;
    for(size_t i=0;i<a.size();++i)
        if(a[i].start!=b[i].start||a[i].end!=b[i].end) return false;
    return true;
}
static std::vector<std::string> p8d_graph_tensor_names() {
    std::vector<std::string> n={"token_embd.weight","output_norm.weight","output.weight"};
    auto nm=[](uint32_t l,const char* s){return std::string("blk.")+std::to_string(l)+s;};
    for(uint32_t l=0;l<28u;++l) {
        n.push_back(nm(l,".attn_norm.weight"));
        n.push_back(nm(l,".attn_q.weight"));
        n.push_back(nm(l,".attn_k.weight"));
        n.push_back(nm(l,".attn_v.weight"));
        n.push_back(nm(l,".attn_q.bias"));
        n.push_back(nm(l,".attn_k.bias"));
        n.push_back(nm(l,".attn_v.bias"));
        n.push_back(nm(l,".attn_output.weight"));
        n.push_back(nm(l,".ffn_norm.weight"));
        n.push_back(nm(l,".ffn_gate.weight"));
        n.push_back(nm(l,".ffn_up.weight"));
        n.push_back(nm(l,".ffn_down.weight"));
    }
    return n;
}
static void p8d_require_graph_contract(const GgufInfo& g) {
    const uint32_t H=3584,KV=512,FFN=18944,V=152064,F32=0,Q4=12,Q6=14;
    if(g.tensor_count!=339u||g.tensors.size()!=339u)
        throw std::runtime_error("P8-D expects exact 339-tensor target");
    auto bc=g.scalars.find("qwen2.block_count");
    if(bc==g.scalars.end()||std::stoul(bc->second)!=28u)
        throw std::runtime_error("P8-D block_count mismatch");
    require_dims(find_tensor(g,"token_embd.weight"),Q4,H,V,"token_embd.weight");
    require_vec(find_tensor(g,"output_norm.weight"),F32,H,"output_norm.weight");
    require_dims(find_tensor(g,"output.weight"),Q6,H,V,"output.weight");
    auto nm=[](uint32_t l,const char* s){return std::string("blk.")+std::to_string(l)+s;};
    for(uint32_t l=0;l<28u;++l) {
        auto AN=nm(l,".attn_norm.weight"),QW=nm(l,".attn_q.weight"),KW=nm(l,".attn_k.weight"),VW=nm(l,".attn_v.weight");
        auto QB=nm(l,".attn_q.bias"),KB=nm(l,".attn_k.bias"),VB=nm(l,".attn_v.bias"),OW=nm(l,".attn_output.weight");
        auto FN=nm(l,".ffn_norm.weight"),GW=nm(l,".ffn_gate.weight"),UW=nm(l,".ffn_up.weight"),DW=nm(l,".ffn_down.weight");
        require_vec(find_tensor(g,AN),F32,H,AN.c_str());
        require_quant_dims(find_tensor(g,QW),H,H,QW.c_str());
        require_quant_dims(find_tensor(g,KW),H,KV,KW.c_str());
        require_quant_dims(find_tensor(g,VW),H,KV,VW.c_str());
        require_vec(find_tensor(g,QB),F32,H,QB.c_str());
        require_vec(find_tensor(g,KB),F32,KV,KB.c_str());
        require_vec(find_tensor(g,VB),F32,KV,VB.c_str());
        require_quant_dims(find_tensor(g,OW),H,H,OW.c_str());
        require_vec(find_tensor(g,FN),F32,H,FN.c_str());
        require_quant_dims(find_tensor(g,GW),H,FFN,GW.c_str());
        require_quant_dims(find_tensor(g,UW),H,FFN,UW.c_str());
        require_quant_dims(find_tensor(g,DW),FFN,H,DW.c_str());
    }
}
static std::map<std::string,TensorBindingDescriptor> p8d_build_graph_bindings(
    const GgufInfo& g,const std::vector<P8DArenaPlan>& arenas,
    uint32_t& missing,uint32_t& ambiguous,uint32_t& piece_count,uint32_t& multi_count,
    bool& span_equivalence,bool& global_coverage) {
    missing=ambiguous=piece_count=multi_count=0;
    span_equivalence=true; global_coverage=true;
    auto names=p8d_graph_tensor_names();
    std::set<std::string> uniq(names.begin(),names.end());
    if(names.size()!=339u||uniq.size()!=339u)
        throw std::runtime_error("P8-D graph name census mismatch");
    std::map<std::string,TensorBindingDescriptor> out;
    struct Span { uint64_t s,e; std::string n; };
    std::vector<Span> all;
    for(const auto& name:names) {
        const auto* t=find_tensor(g,name);
        if(!t) { ++missing; continue; }
        TensorBindingDescriptor d;
        d.name=name; d.ggml_type=t->ggml_type; d.row_bytes=p8d_row_bytes(*t);
        d.rows=p8d_rows_of(*t); d.tensor_bytes=tensor_nbytes(*t);
        auto pieces=p8d_tensor_pieces(*t);
        uint64_t row_cursor=0,src_cursor=t->offset;
        for(auto p:pieces) {
            uint32_t hit_count=0,hit=0; uint64_t base=0;
            for(uint32_t ai=0;ai<uint32_t(arenas.size());++ai) {
                if(p.first>=arenas[ai].start&&p.second<=arenas[ai].end) {
                    ++hit_count; hit=ai; base=p.first-arenas[ai].start;
                }
            }
            if(hit_count!=1u) { ++ambiguous; span_equivalence=false; continue; }
            const uint64_t bytes=p.second-p.first;
            if(bytes%d.row_bytes) { span_equivalence=false; continue; }
            const uint64_t rc=bytes/d.row_bytes;
            d.slices.push_back({hit,base,p.first,bytes,row_cursor,rc});
            if(p.first!=src_cursor||arenas[hit].start+base!=p.first) span_equivalence=false;
            src_cursor=p.second; row_cursor+=rc;
            all.push_back({p.first,p.second,name});
            ++piece_count;
        }
        if(src_cursor!=t->offset+d.tensor_bytes||row_cursor!=d.rows) span_equivalence=false;
        if(d.slices.size()>1u) ++multi_count;
        out.emplace(name,std::move(d));
    }
    if(out.size()!=339u) span_equivalence=false;
    std::sort(all.begin(),all.end(),[](const Span& a,const Span& b){return a.s<b.s;});
    uint64_t cursor=0;
    for(const auto& s:all) {
        if(s.s!=cursor||s.e<=s.s) { global_coverage=false; break; }
        cursor=s.e;
    }
    if(cursor!=4677120000ull) global_coverage=false;
    return out;
}
static bool p8d_row_mapping_equivalent(
    const TensorBindingDescriptor& b,const std::vector<P8DArenaPlan>& arenas,
    uint32_t row,uint64_t tensor_offset) {
    for(const auto& s:b.slices) {
        if(uint64_t(row)>=s.row_start&&uint64_t(row)<s.row_start+s.row_count) {
            const uint64_t local=uint64_t(row)-s.row_start;
            const uint64_t via_source=s.source_start+local*b.row_bytes;
            const uint64_t via_arena=arenas[s.arena].start+s.arena_byte_base+local*b.row_bytes;
            const uint64_t direct=tensor_offset+uint64_t(row)*b.row_bytes;
            return via_source==direct&&via_arena==direct;
        }
    }
    return false;
}
static std::string p8d_json_escape(const std::string& s) {
    std::string o;
    for(char c:s) {
        if(c=='\\'||c=='"') { o+='\\'; o+=c; }
        else if(c=='\n') o+="\\n";
        else if(c=='\r') o+="\\r";
        else o+=c;
    }
    return o;
}

int main(int argc,char** argv) {
    std::string model,shader_dir,plan_path,parent_access,parent_summary,out="p8d_graph_binding_results.json";
    try {
        for(int i=1;i<argc;++i) {
            std::string a=argv[i];
            auto need=[&](const char* f){
                if(i+1>=argc) throw std::runtime_error(std::string("missing value for ")+f);
                return std::string(argv[++i]);
            };
            if(a=="--model") model=need("--model");
            else if(a=="--shader-dir") shader_dir=need("--shader-dir");
            else if(a=="--plan") plan_path=need("--plan");
            else if(a=="--parent-access") parent_access=need("--parent-access");
            else if(a=="--parent-summary") parent_summary=need("--parent-summary");
            else if(a=="--out") out=need("--out");
        }
        if(model.empty()||shader_dir.empty()||plan_path.empty()||parent_access.empty()||parent_summary.empty())
            throw std::runtime_error("--model --shader-dir --plan --parent-access --parent-summary are required");

        const std::string plan_text=p8d_read_text(plan_path);
        const std::string access_text=p8d_read_text(parent_access);
        const std::string summary_text=p8d_read_text(parent_summary);
        if(plan_text.find("\"schema\":\"arcllm.p8a2.segmented_memory_plan.v1\"")==std::string::npos||
           plan_text.find("\"arena_plan\":{\"cap_bytes\":268435456,\"count\":19,\"piece_count\":341")==std::string::npos||
           plan_text.find("\"addressability\":{\"embedding_global_row_to_segment_local_row\":true,\"lm_head_global_row_to_segment_local_row\":true,\"pass\":true}")==std::string::npos)
            throw std::runtime_error("P8-D P8-A2 authoritative plan semantic mismatch");
        if(access_text.find("\"schema\":\"arcllm.p8c.segmented_access_correctness.v1\"")==std::string::npos||
           access_text.find("\"status\":\"PASS\"")==std::string::npos||
           access_text.find("\"p8c_pass\":true")==std::string::npos||
           access_text.find("\"mapping_equivalence\":{\"embedding_pass\":true,\"lm_head_pass\":true,\"pass\":true}")==std::string::npos)
            throw std::runtime_error("P8-D P8-C parent access evidence mismatch");
        if(summary_text.find("\"schema\":  \"arcllm.p8c.summary.v1\"")==std::string::npos||
           summary_text.find("\"status\":  \"PASS\"")==std::string::npos||
           summary_text.find("\"p8c_pass\":  true")==std::string::npos||
           summary_text.find("\"full_inference_permitted\":  false")==std::string::npos)
            throw std::runtime_error("P8-D P8-C parent summary evidence mismatch");

        GgufInfo gguf=GgufReader(model).read();
        TensorStore store;
        TensorStoreReport ts=store.inspect(model,gguf);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("P8-D TensorStore invariants failed");
        if(ts.tensor_bytes_total!=4677120000ull)
            throw std::runtime_error("P8-D weight payload mismatch");
        p8d_require_graph_contract(gguf);

        auto arenas_plan=p8d_recompute_arenas(gguf);
        auto frozen_arenas=p8d_expected_arenas();
        const bool arena_plan_pass=p8d_arenas_equal(arenas_plan,frozen_arenas)&&arenas_plan.size()==19u;
        if(!arena_plan_pass) throw std::runtime_error("P8-D exact P8-A2 arena plan mismatch");

        uint32_t missing=0,ambiguous=0,piece_count=0,multi_count=0;
        bool span_equivalence=false,global_coverage=false;
        auto bindings=p8d_build_graph_bindings(
            gguf,arenas_plan,missing,ambiguous,piece_count,multi_count,span_equivalence,global_coverage);
        if(missing||ambiguous||bindings.size()!=339u||piece_count!=341u||multi_count!=2u||
           !span_equivalence||!global_coverage)
            throw std::runtime_error("P8-D exhaustive graph binding gate failed");

        const auto& emb_bind=bindings.at("token_embd.weight");
        const auto& out_bind=bindings.at("output.weight");
        std::set<std::string> multi_names;
        for(const auto& kv:bindings) if(kv.second.slices.size()>1u) multi_names.insert(kv.first);
        const bool segmented_names_pass=
            (multi_names==std::set<std::string>{"token_embd.weight","output.weight"});
        const bool segment_geometry_pass=
            emb_bind.slices.size()==2u&&emb_bind.row_bytes==2016ull&&
            emb_bind.slices[0].row_count==133152ull&&
            emb_bind.slices[0].source_start==0ull&&emb_bind.slices[0].bytes==268434432ull&&
            emb_bind.slices[1].source_start==268434432ull&&emb_bind.slices[1].bytes==38126592ull&&
            out_bind.slices.size()==2u&&out_bind.row_bytes==2940ull&&
            out_bind.slices[0].row_count==91304ull&&
            out_bind.slices[0].source_start==4230051840ull&&out_bind.slices[0].bytes==268433760ull&&
            out_bind.slices[1].source_start==4498485600ull&&out_bind.slices[1].bytes==178634400ull;
        if(!segmented_names_pass||!segment_geometry_pass)
            throw std::runtime_error("P8-D segmented logical tensor identity/geometry mismatch");

        const auto* emb=find_tensor(gguf,"token_embd.weight");
        const auto* outw=find_tensor(gguf,"output.weight");
        const uint32_t hidden=3584;
        std::vector<uint32_t> emb_ids={0u,1u,133150u,133151u,133152u,133153u,152062u,152063u};
        std::vector<uint32_t> lm_rows={0u,1u,91302u,91303u,91304u,91305u,152062u,152063u};

        bool emb_mapping=true,lm_mapping=true;
        for(uint32_t r:emb_ids)
            emb_mapping=emb_mapping&&p8d_row_mapping_equivalent(emb_bind,arenas_plan,r,emb->offset);
        for(uint32_t r:lm_rows)
            lm_mapping=lm_mapping&&p8d_row_mapping_equivalent(out_bind,arenas_plan,r,outw->offset);
        const bool mapping_equivalence_pass=emb_mapping&&lm_mapping;
        if(!mapping_equivalence_pass)
            throw std::runtime_error("P8-D graph-resolver row mapping equivalence failed");

        const uint8_t* payload=store.mapped_base()+gguf.data_offset;
        const uint8_t* emb_ptr=tensor_ptr(store,gguf,emb);
        const uint8_t* out_ptr=tensor_ptr(store,gguf,outw);
        std::vector<float> emb_ref(uint64_t(emb_ids.size())*hidden);
        for(size_t i=0;i<emb_ids.size();++i)
            q4k_dequant_row_p8c(
                emb_ptr+uint64_t(emb_ids[i])*emb_bind.row_bytes,
                emb_ref.data()+uint64_t(i)*hidden,hidden);
        std::vector<float> x(hidden);
        for(uint32_t i=0;i<hidden;++i)
            x[i]=0.5f*std::sin(float(i)*0.013f)+0.25f*std::cos(float(i)*0.007f);
        std::vector<float> lm_ref(lm_rows.size());
        for(size_t i=0;i<lm_rows.size();++i)
            lm_ref[i]=q6k_dot_row(
                out_ptr+uint64_t(lm_rows[i])*out_bind.row_bytes,x.data(),hidden);

        VkRuntime vk;
        vk.init();
        std::vector<Buffer> arenas;
        arenas.reserve(arenas_plan.size());
        for(const auto& a:arenas_plan)
            arenas.push_back(vk.make_buffer(a.end-a.start,payload+a.start));
        if(arenas.size()!=19u)
            throw std::runtime_error("P8-D Vulkan arena residency count mismatch");

        if(emb_bind.slices[0].arena_byte_base!=0ull||
           emb_bind.slices[1].arena_byte_base!=0ull||
           out_bind.slices[0].arena_byte_base!=0ull||
           out_bind.slices[1].arena_byte_base!=0ull)
            throw std::runtime_error("P8-D endpoint shaders require segment-at-arena-base invariant");

        Buffer b_emb_ids=vk.make_buffer(emb_ids.size()*sizeof(uint32_t),emb_ids.data());
        Buffer b_lm_rows=vk.make_buffer(lm_rows.size()*sizeof(uint32_t),lm_rows.data());
        Buffer b_x=vk.make_buffer(x.size()*sizeof(float),x.data());
        Buffer b_emb_y=vk.make_buffer(emb_ref.size()*sizeof(float));
        Buffer b_lm_y=vk.make_buffer(lm_ref.size()*sizeof(float));

        struct PC { uint32_t n,count,row_bytes,boundary; };
        PC pe{hidden,uint32_t(emb_ids.size()),uint32_t(emb_bind.row_bytes),
              uint32_t(emb_bind.slices[0].row_count)};
        PC pl{hidden,uint32_t(lm_rows.size()),uint32_t(out_bind.row_bytes),
              uint32_t(out_bind.slices[0].row_count)};

        Buffer* emb0=&arenas.at(emb_bind.slices[0].arena);
        Buffer* emb1=&arenas.at(emb_bind.slices[1].arena);
        Buffer* out0=&arenas.at(out_bind.slices[0].arena);
        Buffer* out1=&arenas.at(out_bind.slices[1].arena);

        std::vector<DispatchOp> ops;
        ops.push_back({
            "graph_resolved_segmented_embedding",
            join_path_p8c(shader_dir,"p8c_embedding_q4k_segmented_probe.spv"),
            {emb0,emb1,&b_emb_ids,&b_emb_y},
            push_bytes(pe),
            uint32_t((uint64_t(hidden)*emb_ids.size()+255u)/256u),1,1
        });
        ops.push_back({
            "graph_resolved_segmented_lmhead",
            join_path_p8c(shader_dir,"p8c_lmhead_q6k_segmented_probe.spv"),
            {out0,out1,&b_lm_rows,&b_x,&b_lm_y},
            push_bytes(pl),
            uint32_t((lm_rows.size()+63u)/64u),1,1
        });

        PreparedChain chain=vk.prepare_chain(ops);
        ChainStats stats=vk.execute_prepared(chain,ops);

        std::vector<float> emb_got(emb_ref.size()),lm_got(lm_ref.size());
        std::memcpy(emb_got.data(),b_emb_y.mapped,emb_got.size()*sizeof(float));
        std::memcpy(lm_got.data(),b_lm_y.mapped,lm_got.size()*sizeof(float));
        Metrics em=compare_vec(emb_ref,emb_got,0.02,0.005);
        Metrics lm=compare_vec(lm_ref,lm_got,0.02,0.005);

        bool finite=true;
        for(float v:emb_got) finite=finite&&std::isfinite(v);
        for(float v:lm_got) finite=finite&&std::isfinite(v);
        const bool boundary_pass=
            emb_ids[3]==133151u&&emb_ids[4]==133152u&&
            lm_rows[3]==91303u&&lm_rows[4]==91304u;
        const uint32_t decoder_layer_dispatches=0u;

        const bool pass=
            arena_plan_pass&&segmented_names_pass&&segment_geometry_pass&&
            missing==0u&&ambiguous==0u&&piece_count==341u&&multi_count==2u&&
            span_equivalence&&global_coverage&&mapping_equivalence_pass&&
            boundary_pass&&em.pass&&lm.pass&&finite&&
            stats.dispatch_count==2u&&stats.submit_count==1u&&
            decoder_layer_dispatches==0u;

        std::ofstream o(out,std::ios::binary);
        if(!o) throw std::runtime_error("cannot write P8-D result JSON");
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.p8d.graph_binding.v1\",\n";
        o<<"  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";
        o<<"  \"target\":{\"tensor_count\":"<<gguf.tensor_count
         <<",\"layers\":28,\"hidden\":3584,\"q_heads\":28,\"kv_heads\":4,"
           "\"head_dim\":128,\"ffn\":18944,\"vocab\":152064},\n";
        o<<"  \"parent\":{\"p8a2_plan_loaded\":true,\"p8c_access_loaded\":true,"
           "\"p8c_summary_loaded\":true},\n";
        o<<"  \"resolver\":{\"graph_required_tensors\":339,\"resolved_tensors\":"
         <<bindings.size()<<",\"missing\":"<<missing<<",\"ambiguous\":"<<ambiguous
         <<",\"arena_count\":"<<arenas_plan.size()<<",\"physical_piece_count\":"
         <<piece_count<<",\"segmented_logical_tensor_count\":"<<multi_count
         <<",\"ordinary_single_piece_count\":"<<(bindings.size()-multi_count)
         <<",\"span_equivalence_pass\":"<<json_bool(span_equivalence)
         <<",\"global_coverage_pass\":"<<json_bool(global_coverage)
         <<",\"segmented_names_pass\":"<<json_bool(segmented_names_pass)
         <<",\"segment_geometry_pass\":"<<json_bool(segment_geometry_pass)<<"},\n";
        o<<"  \"mapping_equivalence\":{\"embedding_pass\":"<<json_bool(emb_mapping)
         <<",\"lm_head_pass\":"<<json_bool(lm_mapping)
         <<",\"pass\":"<<json_bool(mapping_equivalence_pass)<<"},\n";
        o<<"  \"probe_rows\":{\"embedding\":[0,1,133150,133151,133152,133153,152062,152063],"
           "\"lm_head\":[0,1,91302,91303,91304,91305,152062,152063],"
           "\"boundary_adjacency_pass\":"<<json_bool(boundary_pass)<<"},\n";
        o<<"  \"embedding\":{\"compared_values\":"<<emb_ref.size()
         <<",\"max_abs\":"<<std::setprecision(12)<<em.max_abs
         <<",\"rmse\":"<<em.rmse<<",\"pass\":"<<json_bool(em.pass)<<"},\n";
        o<<"  \"lm_head\":{\"compared_values\":"<<lm_ref.size()
         <<",\"max_abs\":"<<lm.max_abs<<",\"rmse\":"<<lm.rmse
         <<",\"pass\":"<<json_bool(lm.pass)<<"},\n";
        o<<"  \"execution\":{\"dispatches\":"<<stats.dispatch_count
         <<",\"submits\":"<<stats.submit_count
         <<",\"fence_waits\":"<<stats.fence_wait_count
         <<",\"decoder_layer_dispatches\":"<<decoder_layer_dispatches
         <<",\"finite_pass\":"<<json_bool(finite)<<"},\n";
        o<<"  \"gate\":{\"arena_plan_pass\":"<<json_bool(arena_plan_pass)
         <<",\"resolver_pass\":"
         <<json_bool(missing==0u&&ambiguous==0u&&piece_count==341u&&multi_count==2u&&span_equivalence&&global_coverage)
         <<",\"mapping_equivalence_pass\":"<<json_bool(mapping_equivalence_pass)
         <<",\"embedding_pass\":"<<json_bool(em.pass)
         <<",\"lm_head_pass\":"<<json_bool(lm.pass)
         <<",\"boundary_pass\":"<<json_bool(boundary_pass)
         <<",\"finite_pass\":"<<json_bool(finite)
         <<",\"exact_two_dispatches_pass\":"<<json_bool(stats.dispatch_count==2u)
         <<",\"exact_one_submit_pass\":"<<json_bool(stats.submit_count==1u)
         <<",\"no_decoder_layer_execution_pass\":"<<json_bool(decoder_layer_dispatches==0u)
         <<",\"p8d_pass\":"<<json_bool(pass)<<"}\n";
        o<<"}\n";
        o.close();

        std::cout<<"ArcLLM P8-D graph binding integration\n";
        std::cout<<"resolved="<<bindings.size()<<" pieces="<<piece_count
                 <<" segmented="<<multi_count<<" arenas="<<arenas_plan.size()<<"\n";
        std::cout<<"embedding max_abs="<<em.max_abs<<" rmse="<<em.rmse
                 <<" pass="<<em.pass<<"\n";
        std::cout<<"lm_head max_abs="<<lm.max_abs<<" rmse="<<lm.rmse
                 <<" pass="<<lm.pass<<"\n";
        std::cout<<"dispatches="<<stats.dispatch_count<<" submits="<<stats.submit_count
                 <<" decoder_layer_dispatches="<<decoder_layer_dispatches<<"\n";
        std::cout<<"P8-D "<<(pass?"PASS":"FAIL")<<"\n";

        vk.destroy_prepared(chain);
        vk.destroy_buffer(b_lm_y);
        vk.destroy_buffer(b_emb_y);
        vk.destroy_buffer(b_x);
        vk.destroy_buffer(b_lm_rows);
        vk.destroy_buffer(b_emb_ids);
        for(auto& b:arenas) vk.destroy_buffer(b);
        return pass?0:20;
    } catch(const std::exception& e) {
        std::ofstream o(out,std::ios::binary);
        if(o) {
            o<<"{\n  \"schema\":\"arcllm.p8d.graph_binding.v1\",\n"
               "  \"status\":\"ERROR\",\n  \"error\":\""
             <<p8d_json_escape(e.what())<<"\"\n}\n";
        }
        std::cerr<<e.what()<<"\n";
        return 2;
    }
}
