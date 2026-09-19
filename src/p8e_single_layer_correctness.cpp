#define main p8c_main_disabled
#include "p8c_segmented_access_correctness.cpp"
#undef main

#include <map>
#include <set>

struct P8EArena { uint64_t start=0,end=0; };
struct P8ESlice {
    uint32_t arena=0;
    uint64_t arena_byte_base=0;
    uint64_t source_start=0;
    uint64_t bytes=0;
    uint64_t row_start=0;
    uint64_t row_count=0;
};
struct P8EBinding {
    std::string name;
    uint32_t ggml_type=0;
    uint64_t row_bytes=0;
    uint64_t rows=0;
    uint64_t tensor_bytes=0;
    std::vector<P8ESlice> slices;
};
static constexpr uint64_t P8E_ARENA_CAP=268435456ull;

static std::string p8e_read_text(const std::string& path){
    std::ifstream f(path,std::ios::binary);
    if(!f)throw std::runtime_error("cannot open parent evidence: "+path);
    std::ostringstream ss;ss<<f.rdbuf();return ss.str();
}
static uint64_t p8e_rows_of(const GgufTensorInfo& t){
    if(t.dims.empty())throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t r=1;for(size_t i=1;i<t.dims.size();++i)r*=t.dims[i];return r;
}
static uint64_t p8e_row_bytes(const GgufTensorInfo& t){
    if(t.dims.empty())throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t n=t.dims[0];
    if(t.ggml_type==0u)return n*4ull;
    if(t.ggml_type==12u){if(n%256ull)throw std::runtime_error("bad Q4_K row: "+t.name);return (n/256ull)*144ull;}
    if(t.ggml_type==14u){if(n%256ull)throw std::runtime_error("bad Q6_K row: "+t.name);return (n/256ull)*210ull;}
    throw std::runtime_error("unsupported P8-E tensor type: "+t.name);
}
static std::vector<P8EArena> p8e_expected_arenas(){
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
static std::vector<std::pair<uint64_t,uint64_t>> p8e_pieces(const GgufTensorInfo& t){
    uint64_t rb=p8e_row_bytes(t),rows=p8e_rows_of(t),bytes=rb*rows;
    std::vector<std::pair<uint64_t,uint64_t>> out;
    if(bytes<=P8E_ARENA_CAP){out.push_back({t.offset,t.offset+bytes});return out;}
    uint64_t mr=P8E_ARENA_CAP/rb;if(!mr)throw std::runtime_error("row exceeds arena cap");
    for(uint64_t row=0;row<rows;){
        uint64_t cnt=(std::min)(mr,rows-row);
        uint64_t s=t.offset+row*rb,e=s+cnt*rb;
        out.push_back({s,e});row+=cnt;
    }
    return out;
}
static std::vector<P8EArena> p8e_recompute_arenas(const GgufInfo& g){
    struct Piece{uint64_t s,e;};
    std::vector<const GgufTensorInfo*> ts;for(const auto& t:g.tensors)ts.push_back(&t);
    std::sort(ts.begin(),ts.end(),[](auto*a,auto*b){return a->offset<b->offset;});
    std::vector<Piece> pieces;uint64_t payload=0;
    for(auto*t:ts){
        for(auto p:p8e_pieces(*t))pieces.push_back({p.first,p.second});
        payload=(std::max)(payload,t->offset+tensor_nbytes(*t));
    }
    std::sort(pieces.begin(),pieces.end(),[](const Piece&a,const Piece&b){return a.s<b.s;});
    std::vector<P8EArena> out;uint64_t start=0;
    for(const auto&p:pieces){
        if(p.e-start>P8E_ARENA_CAP){
            if(p.s<=start)throw std::runtime_error("cannot pack P8-E piece");
            out.push_back({start,p.s});start=p.s;
        }
        if(p.e-start>P8E_ARENA_CAP)throw std::runtime_error("piece exceeds P8-E arena");
    }
    if(payload>start)out.push_back({start,payload});
    return out;
}
static bool p8e_arena_equal(const std::vector<P8EArena>&a,const std::vector<P8EArena>&b){
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(a[i].start!=b[i].start||a[i].end!=b[i].end)return false;
    return true;
}
static std::vector<std::string> p8e_graph_names(){
    std::vector<std::string> n={"token_embd.weight","output_norm.weight","output.weight"};
    auto nm=[](uint32_t l,const char*s){return std::string("blk.")+std::to_string(l)+s;};
    for(uint32_t l=0;l<28u;++l){
        n.push_back(nm(l,".attn_norm.weight"));n.push_back(nm(l,".attn_q.weight"));
        n.push_back(nm(l,".attn_k.weight"));n.push_back(nm(l,".attn_v.weight"));
        n.push_back(nm(l,".attn_q.bias"));n.push_back(nm(l,".attn_k.bias"));n.push_back(nm(l,".attn_v.bias"));
        n.push_back(nm(l,".attn_output.weight"));n.push_back(nm(l,".ffn_norm.weight"));
        n.push_back(nm(l,".ffn_gate.weight"));n.push_back(nm(l,".ffn_up.weight"));n.push_back(nm(l,".ffn_down.weight"));
    }
    return n;
}
static std::map<std::string,P8EBinding> p8e_build_bindings(const GgufInfo&g,const std::vector<P8EArena>&arenas,
                                                            uint32_t&pieces,uint32_t&multi,bool&span,bool&coverage){
    pieces=multi=0;span=coverage=true;
    auto names=p8e_graph_names();std::set<std::string> uniq(names.begin(),names.end());
    if(names.size()!=339u||uniq.size()!=339u)throw std::runtime_error("P8-E graph census definition mismatch");
    std::map<std::string,P8EBinding> out;
    struct S{uint64_t a,b;};std::vector<S> all;
    for(const auto&name:names){
        const auto*t=find_tensor(g,name);if(!t)throw std::runtime_error("missing graph tensor: "+name);
        P8EBinding d;d.name=name;d.ggml_type=t->ggml_type;d.row_bytes=p8e_row_bytes(*t);d.rows=p8e_rows_of(*t);d.tensor_bytes=tensor_nbytes(*t);
        uint64_t row=0,cursor=t->offset;
        for(auto p:p8e_pieces(*t)){
            uint32_t hits=0,ai=0;uint64_t base=0;
            for(uint32_t j=0;j<uint32_t(arenas.size());++j)if(p.first>=arenas[j].start&&p.second<=arenas[j].end){++hits;ai=j;base=p.first-arenas[j].start;}
            if(hits!=1u)throw std::runtime_error("ambiguous P8-E binding: "+name);
            uint64_t bytes=p.second-p.first;if(bytes%d.row_bytes)throw std::runtime_error("non-row-aligned P8-E piece");
            uint64_t rc=bytes/d.row_bytes;
            d.slices.push_back({ai,base,p.first,bytes,row,rc});
            if(p.first!=cursor||arenas[ai].start+base!=p.first)span=false;
            cursor=p.second;row+=rc;all.push_back({p.first,p.second});++pieces;
        }
        if(cursor!=t->offset+d.tensor_bytes||row!=d.rows)span=false;
        if(d.slices.size()>1u)++multi;
        out.emplace(name,std::move(d));
    }
    std::sort(all.begin(),all.end(),[](const S&a,const S&b){return a.a<b.a;});
    uint64_t cur=0;for(const auto&s:all){if(s.a!=cur||s.b<=s.a){coverage=false;break;}cur=s.b;}
    if(cur!=4677120000ull)coverage=false;
    return out;
}
static bool p8e_finite(const std::vector<float>&v){for(float x:v)if(!std::isfinite(x))return false;return true;}
static std::vector<float> p8e_matmul(const GgufTensorInfo*t,const uint8_t*w,uint32_t n,uint32_t rows,uint32_t batch,
                                     const std::vector<float>&x,const float*bias){
    if(t->ggml_type==12u)return matmul_q4_cpu(w,n,rows,batch,x,bias);
    if(t->ggml_type==14u)return matmul_q6_cpu(w,n,rows,batch,x,bias);
    throw std::runtime_error("P8-E matmul unsupported type: "+t->name);
}
static std::string p8e_escape(const std::string&s){
    std::string o;for(char c:s){if(c=='\\'||c=='"'){o+='\\';o+=c;}else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else o+=c;}return o;
}

int main(int argc,char**argv){
    std::string model,shader_dir,parent_graph,parent_shader,parent_summary,out="p8e_single_layer_results.json";
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            auto need=[&](const char*f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};
            if(a=="--model")model=need("--model");else if(a=="--shader-dir")shader_dir=need("--shader-dir");
            else if(a=="--parent-graph")parent_graph=need("--parent-graph");
            else if(a=="--parent-shader")parent_shader=need("--parent-shader");
            else if(a=="--parent-summary")parent_summary=need("--parent-summary");
            else if(a=="--out")out=need("--out");
        }
        if(model.empty()||shader_dir.empty()||parent_graph.empty()||parent_shader.empty()||parent_summary.empty())
            throw std::runtime_error("--model --shader-dir --parent-graph --parent-shader --parent-summary are required");

        const auto pg=p8e_read_text(parent_graph),ps=p8e_read_text(parent_shader),pu=p8e_read_text(parent_summary);
        if(pg.find("\"schema\":\"arcllm.p8d.graph_binding.v1\"")==std::string::npos||pg.find("\"p8d_pass\":true")==std::string::npos||
           pg.find("\"resolved_tensors\":339")==std::string::npos||pg.find("\"physical_piece_count\":341")==std::string::npos)
            throw std::runtime_error("P8-E parent graph evidence semantic mismatch");
        if(ps.find("\"schema\":  \"arcllm.p8d.shader_provenance.v1\"")==std::string::npos)
            throw std::runtime_error("P8-E parent shader provenance semantic mismatch");
        if(pu.find("\"schema\":  \"arcllm.p8d.summary.v1\"")==std::string::npos||pu.find("\"p8d_pass\":  true")==std::string::npos||
           pu.find("\"full_inference_permitted\":  false")==std::string::npos)
            throw std::runtime_error("P8-E parent summary semantic mismatch");

        GgufInfo gguf=GgufReader(model).read();TensorStore store;TensorStoreReport ts=store.inspect(model,gguf);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("P8-E TensorStore invariants failed");
        const uint32_t H=3584,QH=28,KVH=4,HD=128,KV=512,FFN=18944,SEQ=4,MAXCTX=4096,F32=0,Q4=12,Q6=14;
        if(gguf.tensor_count!=339u||gguf.tensors.size()!=339u)throw std::runtime_error("P8-E exact tensor census mismatch");
        auto bc=gguf.scalars.find("qwen2.block_count");if(bc==gguf.scalars.end()||std::stoul(bc->second)!=28u)throw std::runtime_error("block count mismatch");

        auto nm=[](const char*s){return std::string("blk.0")+s;};
        auto AN=nm(".attn_norm.weight"),QW=nm(".attn_q.weight"),KW=nm(".attn_k.weight"),VW=nm(".attn_v.weight");
        auto QB=nm(".attn_q.bias"),KB=nm(".attn_k.bias"),VB=nm(".attn_v.bias"),OW=nm(".attn_output.weight");
        auto FN=nm(".ffn_norm.weight"),GW=nm(".ffn_gate.weight"),UW=nm(".ffn_up.weight"),DW=nm(".ffn_down.weight");
        const auto*an=find_tensor(gguf,AN),*qw=find_tensor(gguf,QW),*kw=find_tensor(gguf,KW),*vw=find_tensor(gguf,VW);
        const auto*qb=find_tensor(gguf,QB),*kb=find_tensor(gguf,KB),*vb=find_tensor(gguf,VB),*ow=find_tensor(gguf,OW);
        const auto*fn=find_tensor(gguf,FN),*gw=find_tensor(gguf,GW),*uw=find_tensor(gguf,UW),*dw=find_tensor(gguf,DW);
        require_vec(an,F32,H,AN.c_str());require_dims(qw,Q4,H,H,QW.c_str());require_dims(kw,Q4,H,KV,KW.c_str());
        require_quant_dims(vw,H,KV,VW.c_str());require_vec(qb,F32,H,QB.c_str());require_vec(kb,F32,KV,KB.c_str());require_vec(vb,F32,KV,VB.c_str());
        require_dims(ow,Q4,H,H,OW.c_str());require_vec(fn,F32,H,FN.c_str());require_dims(gw,Q4,H,FFN,GW.c_str());require_dims(uw,Q4,H,FFN,UW.c_str());
        require_quant_dims(dw,FFN,H,DW.c_str());
        if(vw->ggml_type!=Q4&&vw->ggml_type!=Q6)throw std::runtime_error("attn_v target format obstruction");
        if(dw->ggml_type!=Q4&&dw->ggml_type!=Q6)throw std::runtime_error("ffn_down target format obstruction");

        auto arenas_plan=p8e_recompute_arenas(gguf);if(!p8e_arena_equal(arenas_plan,p8e_expected_arenas()))throw std::runtime_error("P8-E frozen arena mismatch");
        uint32_t piece_count=0,multi_count=0;bool span=false,coverage=false;
        auto bindings=p8e_build_bindings(gguf,arenas_plan,piece_count,multi_count,span,coverage);
        if(bindings.size()!=339u||piece_count!=341u||multi_count!=2u||!span||!coverage)throw std::runtime_error("P8-E graph binding regression");
        std::vector<std::string> layer_names={AN,QW,KW,VW,QB,KB,VB,OW,FN,GW,UW,DW};
        for(const auto&n:layer_names)if(bindings.at(n).slices.size()!=1u)throw std::runtime_error("P8-E layer-0 tensor unexpectedly segmented: "+n);

        float eps=1e-6f;auto ei=gguf.scalars.find("qwen2.attention.layer_norm_rms_epsilon");if(ei!=gguf.scalars.end())eps=std::stof(ei->second);
        float theta=1000000.0f;auto ri=gguf.scalars.find("qwen2.rope.freq_base");if(ri!=gguf.scalars.end())theta=std::stof(ri->second);

        std::vector<float>x(uint64_t(SEQ)*H);
        for(size_t i=0;i<x.size();++i)x[i]=0.17f*std::sin(float(i+11u)*0.009f)+0.03f*std::cos(float(i+5u)*0.017f);
        const uint8_t* payload=store.mapped_base()+gguf.data_offset;
        auto TP=[&](const GgufTensorInfo*t){return tensor_ptr(store,gguf,t);};
        const float* an_w=reinterpret_cast<const float*>(TP(an));
        const float* qb_w=reinterpret_cast<const float*>(TP(qb));
        const float* kb_w=reinterpret_cast<const float*>(TP(kb));
        const float* vb_w=reinterpret_cast<const float*>(TP(vb));
        const float* fn_w=reinterpret_cast<const float*>(TP(fn));

        auto ref_n1=rmsnorm_cpu(x,an_w,SEQ,H,eps);
        auto ref_q=matmul_q4_cpu(TP(qw),H,H,SEQ,ref_n1,qb_w);
        auto ref_k=matmul_q4_cpu(TP(kw),H,KV,SEQ,ref_n1,kb_w);
        auto ref_v=p8e_matmul(vw,TP(vw),H,KV,SEQ,ref_n1,vb_w);
        auto ref_qr=ref_q;rope_cpu(ref_qr,SEQ,QH,HD,0,theta);
        auto ref_kr=ref_k;rope_cpu(ref_kr,SEQ,KVH,HD,0,theta);
        auto ref_attn=attention_cpu(ref_qr,ref_kr,ref_v,SEQ,QH,KVH,HD);
        auto ref_o=matmul_q4_cpu(TP(ow),H,H,SEQ,ref_attn,nullptr);
        auto ref_r1=add_cpu(x,ref_o);
        auto ref_n2=rmsnorm_cpu(ref_r1,fn_w,SEQ,H,eps);
        auto ref_g=matmul_q4_cpu(TP(gw),H,FFN,SEQ,ref_n2,nullptr);
        auto ref_u=matmul_q4_cpu(TP(uw),H,FFN,SEQ,ref_n2,nullptr);
        auto ref_s=swiglu_cpu(ref_g,ref_u);
        auto ref_d=p8e_matmul(dw,TP(dw),FFN,H,SEQ,ref_s,nullptr);
        auto ref_out=add_cpu(ref_r1,ref_d);

        VkRuntime vk;vk.init();std::vector<Buffer> arenas;arenas.reserve(19);
        for(const auto&a:arenas_plan)arenas.push_back(vk.make_buffer(a.end-a.start,payload+a.start));
        auto BIND=[&](const std::string&n)->const P8EBinding&{return bindings.at(n);};
        auto AB=[&](const std::string&n)->Buffer*{const auto&b=BIND(n);return &arenas.at(b.slices[0].arena);};
        auto BASE=[&](const std::string&n)->uint32_t{uint64_t x=BIND(n).slices[0].arena_byte_base;if(x>0xffffffffull)throw std::runtime_error("P8-E base exceeds uint32");return uint32_t(x);};
        auto FBASE=[&](const std::string&n)->uint32_t{uint32_t b=BASE(n);if(b%4u)throw std::runtime_error("unaligned F32 binding");return b/4u;};
        float zero=0.0f;auto fb=[&](uint64_t n){return vk.make_buffer(n*sizeof(float));};
        Buffer b_x=vk.make_buffer(x.size()*sizeof(float),x.data()),b_dummy=vk.make_buffer(sizeof(float),&zero);
        Buffer b_n1=fb(uint64_t(SEQ)*H),b_q=fb(uint64_t(SEQ)*H),b_k=fb(uint64_t(SEQ)*KV),b_v=fb(uint64_t(SEQ)*KV);
        Buffer b_qr=fb(uint64_t(SEQ)*H),b_kr=fb(uint64_t(SEQ)*KV),b_attn=fb(uint64_t(SEQ)*H),b_o=fb(uint64_t(SEQ)*H);
        Buffer b_r1=fb(uint64_t(SEQ)*H),b_n2=fb(uint64_t(SEQ)*H),b_g=fb(uint64_t(SEQ)*FFN),b_u=fb(uint64_t(SEQ)*FFN);
        Buffer b_s=fb(uint64_t(SEQ)*FFN),b_d=fb(uint64_t(SEQ)*H),b_out=fb(uint64_t(SEQ)*H);
        Buffer b_kcache=fb(uint64_t(MAXCTX)*KV),b_vcache=fb(uint64_t(MAXCTX)*KV);

        struct PCRms{uint32_t n,batch;float eps;uint32_t w_base;};
        struct PCGemm{uint32_t n,rows,batch,row_bytes,add_bias,w_base_bytes,bias_base;};
        struct PCFusedGU{uint32_t n,rows,batch,row_bytes,gate_base_bytes,up_base_bytes;};
        struct PCRope{uint32_t batch,heads,head_dim,pos_base;float theta;};
        struct PCKV{uint32_t layer,cache_pos_start,batch,max_ctx,kv_dim;};
        struct PCAttn{uint32_t q_heads,kv_heads,seq,dim;float scale;};
        struct PCN{uint32_t n;};
        std::vector<DispatchOp> ops;
        auto addop=[&](const std::string&name,const std::string&spv,std::vector<Buffer*>bufs,std::vector<uint8_t>push,uint32_t gx,uint32_t gy=1){
            ops.push_back({name,join_path_p8c(shader_dir,spv),std::move(bufs),std::move(push),gx,gy,1});
        };
        addop("L0.attn_rmsnorm","p7_rmsnorm_seq.spv",{&b_x,AB(AN),&b_n1},push_bytes(PCRms{H,SEQ,eps,FBASE(AN)}),SEQ);
        addop("L0.q_proj","p7c_ffn_q4k_tiled.spv",{AB(QW),&b_n1,AB(QB),&b_q},push_bytes(PCGemm{H,H,SEQ,q4_row_bytes(H),1,BASE(QW),FBASE(QB)}),(H+7u)/8u,(SEQ+7u)/8u);
        addop("L0.k_proj","p7c_ffn_q4k_tiled.spv",{AB(KW),&b_n1,AB(KB),&b_k},push_bytes(PCGemm{H,KV,SEQ,q4_row_bytes(H),1,BASE(KW),FBASE(KB)}),(KV+7u)/8u,(SEQ+7u)/8u);
        uint32_t vrb=vw->ggml_type==Q4?q4_row_bytes(H):q6_row_bytes(H);
        addop("L0.v_proj",vw->ggml_type==Q4?"p7c_ffn_q4k_tiled.spv":"p7c_ffn_q6k_tiled.spv",{AB(VW),&b_n1,AB(VB),&b_v},push_bytes(PCGemm{H,KV,SEQ,vrb,1,BASE(VW),FBASE(VB)}),(KV+7u)/8u,(SEQ+7u)/8u);
        addop("L0.q_rope","p7_rope_seq.spv",{&b_q,&b_qr},push_bytes(PCRope{SEQ,QH,HD,0,theta}),(SEQ*QH*(HD/2u)+127u)/128u);
        addop("L0.k_rope","p7_rope_seq.spv",{&b_k,&b_kr},push_bytes(PCRope{SEQ,KVH,HD,0,theta}),(SEQ*KVH*(HD/2u)+127u)/128u);
        addop("L0.kv_store","p7_kv_store.spv",{&b_kr,&b_v,&b_kcache,&b_vcache},push_bytes(PCKV{0,0,SEQ,MAXCTX,KV}),(SEQ*KV+255u)/256u);
        addop("L0.causal_gqa","p7_attention_prefill_online.spv",{&b_qr,&b_kr,&b_v,&b_attn},push_bytes(PCAttn{QH,KVH,SEQ,HD,1.0f/std::sqrt(float(HD))}),SEQ*QH);
        addop("L0.o_proj","p7c_ffn_q4k_tiled.spv",{AB(OW),&b_attn,&b_dummy,&b_o},push_bytes(PCGemm{H,H,SEQ,q4_row_bytes(H),0,BASE(OW),0}),(H+7u)/8u,(SEQ+7u)/8u);
        addop("L0.attn_residual","p7_add.spv",{&b_x,&b_o,&b_r1},push_bytes(PCN{SEQ*H}),(SEQ*H+255u)/256u);
        addop("L0.ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b_r1,AB(FN),&b_n2},push_bytes(PCRms{H,SEQ,eps,FBASE(FN)}),SEQ);
        addop("L0.ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv",{AB(GW),AB(UW),&b_n2,&b_g,&b_u},push_bytes(PCFusedGU{H,FFN,SEQ,q4_row_bytes(H),BASE(GW),BASE(UW)}),(FFN+7u)/8u,(SEQ+15u)/16u);
        addop("L0.swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(PCN{SEQ*FFN}),(SEQ*FFN+255u)/256u);
        uint32_t drb=dw->ggml_type==Q4?q4_row_bytes(FFN):q6_row_bytes(FFN);
        addop("L0.ffn_down",dw->ggml_type==Q4?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv",{AB(DW),&b_s,&b_dummy,&b_d},push_bytes(PCGemm{FFN,H,SEQ,drb,0,BASE(DW),0}),(H+7u)/8u,(SEQ+15u)/16u);
        addop("L0.ffn_residual","p7_add.spv",{&b_r1,&b_d,&b_out},push_bytes(PCN{SEQ*H}),(SEQ*H+255u)/256u);
        if(ops.size()!=15u)throw std::runtime_error("P8-E operation census mismatch");

        PreparedChain chain=vk.prepare_chain(ops);ChainStats stats=vk.execute_prepared(chain,ops);
        auto readf=[&](Buffer&b,size_t n){std::vector<float>v(n);std::memcpy(v.data(),b.mapped,n*sizeof(float));return v;};
        auto g_n1=readf(b_n1,ref_n1.size()),g_q=readf(b_q,ref_q.size()),g_k=readf(b_k,ref_k.size()),g_v=readf(b_v,ref_v.size());
        auto g_qr=readf(b_qr,ref_qr.size()),g_kr=readf(b_kr,ref_kr.size());
        auto g_kc=readf(b_kcache,ref_kr.size()),g_vc=readf(b_vcache,ref_v.size());
        auto g_attn=readf(b_attn,ref_attn.size()),g_o=readf(b_o,ref_o.size()),g_r1=readf(b_r1,ref_r1.size());
        auto g_n2=readf(b_n2,ref_n2.size()),g_g=readf(b_g,ref_g.size()),g_u=readf(b_u,ref_u.size()),g_s=readf(b_s,ref_s.size());
        auto g_d=readf(b_d,ref_d.size()),g_out=readf(b_out,ref_out.size());

        struct Ck{std::string name;size_t n;Metrics m;bool finite;};
        std::vector<Ck> ck;
        auto cmp=[&](const char*n,const std::vector<float>&r,const std::vector<float>&g){ck.push_back({n,r.size(),compare_vec(r,g,0.02,0.005),p8e_finite(g)});};
        cmp("attn_rmsnorm",ref_n1,g_n1);cmp("q_proj",ref_q,g_q);cmp("k_proj",ref_k,g_k);cmp("v_proj",ref_v,g_v);
        cmp("q_rope",ref_qr,g_qr);cmp("k_rope",ref_kr,g_kr);cmp("k_cache_rows_0_3",ref_kr,g_kc);cmp("v_cache_rows_0_3",ref_v,g_vc);
        cmp("causal_gqa",ref_attn,g_attn);cmp("o_proj",ref_o,g_o);cmp("attn_residual",ref_r1,g_r1);cmp("ffn_rmsnorm",ref_n2,g_n2);
        cmp("ffn_gate",ref_g,g_g);cmp("ffn_up",ref_u,g_u);cmp("swiglu",ref_s,g_s);cmp("ffn_down",ref_d,g_d);cmp("final_layer_output",ref_out,g_out);
        bool checkpoints_pass=true;std::string first_fail;
        for(const auto&c:ck)if(!(c.m.pass&&c.finite)){checkpoints_pass=false;if(first_fail.empty())first_fail=c.name;}
        bool final_pass=ck.back().m.pass&&ck.back().finite;
        bool kv_rows_pass=ck[6].m.pass&&ck[6].finite&&ck[7].m.pass&&ck[7].finite;
        uint32_t executed_layer=0,executed_layer_count=1;
        bool pass=checkpoints_pass&&final_pass&&kv_rows_pass&&stats.dispatch_count==15u&&stats.submit_count==1u&&
                  executed_layer==0u&&executed_layer_count==1u&&piece_count==341u&&multi_count==2u&&span&&coverage;

        std::ofstream o(out,std::ios::binary);if(!o)throw std::runtime_error("cannot write P8-E result JSON");
        o<<"{\n  \"schema\":\"arcllm.p8e.single_layer_correctness.v1\",\n";
        o<<"  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";
        o<<"  \"target\":{\"tensor_count\":339,\"layers\":28,\"hidden\":3584,\"q_heads\":28,\"kv_heads\":4,\"head_dim\":128,\"kv_dim\":512,\"ffn\":18944},\n";
        o<<"  \"scope\":{\"executed_layer\":0,\"executed_layer_count\":1,\"sequence_length\":4,\"position_base\":0,\"embedding_executed\":false,\"lm_head_executed\":false,\"decode_executed\":false,\"generation_executed\":false},\n";
        o<<"  \"binding\":{\"arena_count\":"<<arenas_plan.size()<<",\"physical_piece_count\":"<<piece_count<<",\"segmented_logical_tensor_count\":"<<multi_count<<",\"span_equivalence_pass\":"<<json_bool(span)<<",\"global_coverage_pass\":"<<json_bool(coverage)<<",\"layer0_all_single_piece\":true},\n";
        o<<"  \"formats\":{\"attn_v\":\""<<(vw->ggml_type==Q4?"Q4_K":"Q6_K")<<"\",\"ffn_down\":\""<<(dw->ggml_type==Q4?"Q4_K":"Q6_K")<<"\"},\n";
        o<<"  \"checkpoints\":{\n";
        for(size_t i=0;i<ck.size();++i){const auto&c=ck[i];o<<"    \""<<c.name<<"\":{\"compared_values\":"<<c.n<<",\"max_abs\":"<<std::setprecision(12)<<c.m.max_abs<<",\"rmse\":"<<c.m.rmse<<",\"finite_pass\":"<<json_bool(c.finite)<<",\"pass\":"<<json_bool(c.m.pass&&c.finite)<<"}"<<(i+1<ck.size()?",":"")<<"\n";}
        o<<"  },\n";
        o<<"  \"execution\":{\"dispatches\":"<<stats.dispatch_count<<",\"submits\":"<<stats.submit_count<<",\"fence_waits\":"<<stats.fence_wait_count<<",\"executed_layer\":0,\"executed_layer_count\":1},\n";
        o<<"  \"gate\":{\"binding_pass\":"<<json_bool(piece_count==341u&&multi_count==2u&&span&&coverage)<<",\"checkpoint_pass\":"<<json_bool(checkpoints_pass)<<",\"kv_rows_pass\":"<<json_bool(kv_rows_pass)<<",\"final_layer_output_pass\":"<<json_bool(final_pass)<<",\"exact_15_dispatches_pass\":"<<json_bool(stats.dispatch_count==15u)<<",\"exact_one_submit_pass\":"<<json_bool(stats.submit_count==1u)<<",\"exact_one_layer_pass\":"<<json_bool(executed_layer_count==1u&&executed_layer==0u)<<",\"p8e_pass\":"<<json_bool(pass)<<"},\n";
        o<<"  \"first_failing_checkpoint\":\""<<p8e_escape(first_fail)<<"\"\n}\n";o.close();

        std::cout<<"ArcLLM P8-E bounded single-layer correctness\n";
        std::cout<<"layer=0 seq=4 dispatches="<<stats.dispatch_count<<" submits="<<stats.submit_count<<"\n";
        for(const auto&c:ck)std::cout<<c.name<<" max_abs="<<c.m.max_abs<<" rmse="<<c.m.rmse<<" pass="<<(c.m.pass&&c.finite)<<"\n";
        std::cout<<"P8-E "<<(pass?"PASS":"FAIL");if(!first_fail.empty())std::cout<<" first_fail="<<first_fail;std::cout<<"\n";

        vk.destroy_prepared(chain);
        for(Buffer*b:{&b_vcache,&b_kcache,&b_out,&b_d,&b_s,&b_u,&b_g,&b_n2,&b_r1,&b_o,&b_attn,&b_kr,&b_qr,&b_v,&b_k,&b_q,&b_n1,&b_dummy,&b_x})vk.destroy_buffer(*b);
        for(auto&b:arenas)vk.destroy_buffer(b);
        return pass?0:20;
    }catch(const std::exception&e){
        std::ofstream o(out,std::ios::binary);if(o)o<<"{\n  \"schema\":\"arcllm.p8e.single_layer_correctness.v1\",\n  \"status\":\"ERROR\",\n  \"error\":\""<<p8e_escape(e.what())<<"\"\n}\n";
        std::cerr<<e.what()<<"\n";return 2;
    }
}
