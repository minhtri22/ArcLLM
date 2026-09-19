#define main p8c_main_disabled
#include "p8c_segmented_access_correctness.cpp"
#undef main

#include <map>
#include <set>
#include <array>

struct P8GArena { uint64_t start=0,end=0; };
struct P8GSlice {
    uint32_t arena=0;
    uint64_t arena_byte_base=0;
    uint64_t source_start=0;
    uint64_t bytes=0;
    uint64_t row_start=0;
    uint64_t row_count=0;
};
struct P8GBinding {
    std::string name;
    uint32_t ggml_type=0;
    uint64_t row_bytes=0;
    uint64_t rows=0;
    uint64_t tensor_bytes=0;
    std::vector<P8GSlice> slices;
};
static constexpr uint64_t P8G_ARENA_CAP=268435456ull;

static std::string p8g_read_text(const std::string& path){
    std::ifstream f(path,std::ios::binary);
    if(!f)throw std::runtime_error("cannot open parent evidence: "+path);
    std::ostringstream ss;ss<<f.rdbuf();return ss.str();
}
static uint64_t p8g_rows_of(const GgufTensorInfo& t){
    if(t.dims.empty())throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t r=1;for(size_t i=1;i<t.dims.size();++i)r*=t.dims[i];return r;
}
static uint64_t p8g_row_bytes(const GgufTensorInfo& t){
    if(t.dims.empty())throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t n=t.dims[0];
    if(t.ggml_type==0u)return n*4ull;
    if(t.ggml_type==12u){if(n%256ull)throw std::runtime_error("bad Q4_K row: "+t.name);return (n/256ull)*144ull;}
    if(t.ggml_type==14u){if(n%256ull)throw std::runtime_error("bad Q6_K row: "+t.name);return (n/256ull)*210ull;}
    throw std::runtime_error("unsupported P8-G tensor type: "+t.name);
}
static std::vector<P8GArena> p8g_expected_arenas(){
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
static std::vector<std::pair<uint64_t,uint64_t>> p8g_pieces(const GgufTensorInfo& t){
    uint64_t rb=p8g_row_bytes(t),rows=p8g_rows_of(t),bytes=rb*rows;
    std::vector<std::pair<uint64_t,uint64_t>> out;
    if(bytes<=P8G_ARENA_CAP){out.push_back({t.offset,t.offset+bytes});return out;}
    uint64_t mr=P8G_ARENA_CAP/rb;if(!mr)throw std::runtime_error("row exceeds arena cap");
    for(uint64_t row=0;row<rows;){
        uint64_t cnt=(std::min)(mr,rows-row);
        uint64_t s=t.offset+row*rb,e=s+cnt*rb;
        out.push_back({s,e});row+=cnt;
    }
    return out;
}
static std::vector<P8GArena> p8g_recompute_arenas(const GgufInfo& g){
    struct Piece{uint64_t s,e;};
    std::vector<const GgufTensorInfo*> ts;for(const auto& t:g.tensors)ts.push_back(&t);
    std::sort(ts.begin(),ts.end(),[](auto*a,auto*b){return a->offset<b->offset;});
    std::vector<Piece> pieces;uint64_t payload=0;
    for(auto*t:ts){
        for(auto p:p8g_pieces(*t))pieces.push_back({p.first,p.second});
        payload=(std::max)(payload,t->offset+tensor_nbytes(*t));
    }
    std::sort(pieces.begin(),pieces.end(),[](const Piece&a,const Piece&b){return a.s<b.s;});
    std::vector<P8GArena> out;uint64_t start=0;
    for(const auto&p:pieces){
        if(p.e-start>P8G_ARENA_CAP){
            if(p.s<=start)throw std::runtime_error("cannot pack P8-G piece");
            out.push_back({start,p.s});start=p.s;
        }
        if(p.e-start>P8G_ARENA_CAP)throw std::runtime_error("piece exceeds P8-G arena");
    }
    if(payload>start)out.push_back({start,payload});
    return out;
}
static bool p8g_arena_equal(const std::vector<P8GArena>&a,const std::vector<P8GArena>&b){
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(a[i].start!=b[i].start||a[i].end!=b[i].end)return false;
    return true;
}
static std::vector<std::string> p8g_graph_names(){
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
static std::map<std::string,P8GBinding> p8g_build_bindings(const GgufInfo&g,const std::vector<P8GArena>&arenas,
                                                            uint32_t&pieces,uint32_t&multi,bool&span,bool&coverage){
    pieces=multi=0;span=coverage=true;
    auto names=p8g_graph_names();std::set<std::string> uniq(names.begin(),names.end());
    if(names.size()!=339u||uniq.size()!=339u)throw std::runtime_error("P8-G graph census definition mismatch");
    std::map<std::string,P8GBinding> out;
    struct S{uint64_t a,b;};std::vector<S> all;
    for(const auto&name:names){
        const auto*t=find_tensor(g,name);if(!t)throw std::runtime_error("missing graph tensor: "+name);
        P8GBinding d;d.name=name;d.ggml_type=t->ggml_type;d.row_bytes=p8g_row_bytes(*t);d.rows=p8g_rows_of(*t);d.tensor_bytes=tensor_nbytes(*t);
        uint64_t row=0,cursor=t->offset;
        for(auto p:p8g_pieces(*t)){
            uint32_t hits=0,ai=0;uint64_t base=0;
            for(uint32_t j=0;j<uint32_t(arenas.size());++j)if(p.first>=arenas[j].start&&p.second<=arenas[j].end){++hits;ai=j;base=p.first-arenas[j].start;}
            if(hits!=1u)throw std::runtime_error("ambiguous P8-G binding: "+name);
            uint64_t bytes=p.second-p.first;if(bytes%d.row_bytes)throw std::runtime_error("non-row-aligned P8-G piece");
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
static bool p8g_finite(const std::vector<float>&v){for(float x:v)if(!std::isfinite(x))return false;return true;}
static std::vector<float> p8g_matmul(const GgufTensorInfo*t,const uint8_t*w,uint32_t n,uint32_t rows,uint32_t batch,
                                     const std::vector<float>&x,const float*bias){
    if(t->ggml_type==12u)return matmul_q4_cpu(w,n,rows,batch,x,bias);
    if(t->ggml_type==14u)return matmul_q6_cpu(w,n,rows,batch,x,bias);
    throw std::runtime_error("P8-G matmul unsupported type: "+t->name);
}
static std::string p8g_escape(const std::string&s){
    std::string o;for(char c:s){if(c=='\\'||c=='"'){o+='\\';o+=c;}else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else o+=c;}return o;
}


struct P8GLayerRef {
    std::vector<float> n1,q,k,v,qr,kr,attn,o,r1,n2,g,u,s,d,out;
    uint32_t v_type=0,down_type=0;
};
struct P8GLayerBuffers {
    Buffer n1,q,k,v,qr,kr,attn,o,r1,n2,g,u,s,d,out;
};
struct P8GCheckpoint {
    uint32_t layer=0;
    std::string name;
    size_t n=0;
    Metrics m;
    bool finite=false;
};

int main(int argc,char**argv){
    std::string model,shader_dir,parent_result,parent_shader,parent_summary,out="p8g1_causal_decomposition_results.json";
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            auto need=[&](const char*f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};
            if(a=="--model")model=need("--model");
            else if(a=="--shader-dir")shader_dir=need("--shader-dir");
            else if(a=="--parent-result")parent_result=need("--parent-result");
            else if(a=="--parent-shader")parent_shader=need("--parent-shader");
            else if(a=="--parent-summary")parent_summary=need("--parent-summary");
            else if(a=="--out")out=need("--out");
        }
        if(model.empty()||shader_dir.empty()||parent_result.empty()||parent_shader.empty()||parent_summary.empty())
            throw std::runtime_error("--model --shader-dir --parent-result --parent-shader --parent-summary are required");

        const auto pr=p8g_read_text(parent_result),ps=p8g_read_text(parent_shader),pu=p8g_read_text(parent_summary);
        if(pr.find("arcllm.p8g.four_layer_prefix_correctness.v1")==std::string::npos||
           pr.find("\"status\":\"FAIL\"")==std::string::npos||
           pr.find("\"first_failing_checkpoint\":\"L3.ffn_down\"")==std::string::npos||
           pr.find("\"dispatches\":60")==std::string::npos||
           pr.find("\"executed_layer_count\":4")==std::string::npos)
            throw std::runtime_error("P8-G1 parent P8-G result semantic mismatch");
        if(ps.find("arcllm.p8g.shader_provenance.v1")==std::string::npos)
            throw std::runtime_error("P8-G1 parent P8-G shader provenance semantic mismatch");
        if(pu.find("arcllm.p8g.summary.v1")==std::string::npos||
           pu.find("\"status\":  \"FAIL\"")==std::string::npos)
            throw std::runtime_error("P8-G1 parent P8-G summary semantic mismatch");

        GgufInfo gguf=GgufReader(model).read();TensorStore store;TensorStoreReport ts=store.inspect(model,gguf);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("P8-G1 TensorStore invariants failed");

        const uint32_t H=3584,QH=28,KVH=4,HD=128,KV=512,FFN=18944,SEQ=4,MAXCTX=4096,F32=0,Q4=12,Q6=14,EXEC_LAYERS=4;
        if(gguf.tensor_count!=339u||gguf.tensors.size()!=339u)throw std::runtime_error("P8-G1 exact tensor census mismatch");
        auto bc=gguf.scalars.find("qwen2.block_count");if(bc==gguf.scalars.end()||std::stoul(bc->second)!=28u)throw std::runtime_error("P8-G1 block count mismatch");

        auto arenas_plan=p8g_recompute_arenas(gguf);
        if(!p8g_arena_equal(arenas_plan,p8g_expected_arenas()))throw std::runtime_error("P8-G1 frozen arena mismatch");
        uint32_t piece_count=0,multi_count=0;bool span=false,coverage=false;
        auto bindings=p8g_build_bindings(gguf,arenas_plan,piece_count,multi_count,span,coverage);
        if(bindings.size()!=339u||piece_count!=341u||multi_count!=2u||!span||!coverage)
            throw std::runtime_error("P8-G1 graph binding regression");

        float eps=1e-6f;auto ei=gguf.scalars.find("qwen2.attention.layer_norm_rms_epsilon");
        if(ei!=gguf.scalars.end())eps=std::stof(ei->second);
        float theta=1000000.0f;auto ri=gguf.scalars.find("qwen2.rope.freq_base");
        if(ri!=gguf.scalars.end())theta=std::stof(ri->second);

        auto nm=[](uint32_t l,const char*s){return std::string("blk.")+std::to_string(l)+s;};
        struct LT {
            std::string AN,QW,KW,VW,QB,KB,VB,OW,FN,GW,UW,DW;
            const GgufTensorInfo *an=nullptr,*qw=nullptr,*kw=nullptr,*vw=nullptr,*qb=nullptr,*kb=nullptr,*vb=nullptr,*ow=nullptr,*fn=nullptr,*gw=nullptr,*uw=nullptr,*dw=nullptr;
        };
        std::array<LT,EXEC_LAYERS> lt;
        for(uint32_t l=0;l<EXEC_LAYERS;++l){
            LT z;
            z.AN=nm(l,".attn_norm.weight");z.QW=nm(l,".attn_q.weight");z.KW=nm(l,".attn_k.weight");z.VW=nm(l,".attn_v.weight");
            z.QB=nm(l,".attn_q.bias");z.KB=nm(l,".attn_k.bias");z.VB=nm(l,".attn_v.bias");z.OW=nm(l,".attn_output.weight");
            z.FN=nm(l,".ffn_norm.weight");z.GW=nm(l,".ffn_gate.weight");z.UW=nm(l,".ffn_up.weight");z.DW=nm(l,".ffn_down.weight");
            z.an=find_tensor(gguf,z.AN);z.qw=find_tensor(gguf,z.QW);z.kw=find_tensor(gguf,z.KW);z.vw=find_tensor(gguf,z.VW);
            z.qb=find_tensor(gguf,z.QB);z.kb=find_tensor(gguf,z.KB);z.vb=find_tensor(gguf,z.VB);z.ow=find_tensor(gguf,z.OW);
            z.fn=find_tensor(gguf,z.FN);z.gw=find_tensor(gguf,z.GW);z.uw=find_tensor(gguf,z.UW);z.dw=find_tensor(gguf,z.DW);
            require_vec(z.an,F32,H,z.AN.c_str());require_dims(z.qw,Q4,H,H,z.QW.c_str());require_dims(z.kw,Q4,H,KV,z.KW.c_str());
            require_quant_dims(z.vw,H,KV,z.VW.c_str());require_vec(z.qb,F32,H,z.QB.c_str());require_vec(z.kb,F32,KV,z.KB.c_str());require_vec(z.vb,F32,KV,z.VB.c_str());
            require_dims(z.ow,Q4,H,H,z.OW.c_str());require_vec(z.fn,F32,H,z.FN.c_str());require_dims(z.gw,Q4,H,FFN,z.GW.c_str());require_dims(z.uw,Q4,H,FFN,z.UW.c_str());
            require_quant_dims(z.dw,FFN,H,z.DW.c_str());
            std::vector<std::string> names={z.AN,z.QW,z.KW,z.VW,z.QB,z.KB,z.VB,z.OW,z.FN,z.GW,z.UW,z.DW};
            for(const auto&n:names)if(bindings.at(n).slices.size()!=1u)
                throw std::runtime_error("P8-G1 executed-layer tensor unexpectedly segmented: "+n);
            lt[l]=std::move(z);
        }
        if(lt[3].dw->ggml_type!=Q4)throw std::runtime_error("P8-G1 focus tensor is not Q4_K");
        const auto&focus_binding=bindings.at(lt[3].DW);
        if(focus_binding.slices.size()!=1u)throw std::runtime_error("P8-G1 focus tensor is segmented");
        const auto&focus_slice=focus_binding.slices[0];
        const bool focus_span_exact=
            focus_slice.source_start==lt[3].dw->offset&&
            focus_slice.bytes==tensor_nbytes(*lt[3].dw)&&
            arenas_plan.at(focus_slice.arena).start+focus_slice.arena_byte_base==lt[3].dw->offset;
        if(!focus_span_exact)throw std::runtime_error("P8-G1 focus tensor byte-span mismatch");

        std::vector<float>x(uint64_t(SEQ)*H);
        for(size_t i=0;i<x.size();++i)x[i]=0.17f*std::sin(float(i+11u)*0.009f)+0.03f*std::cos(float(i+5u)*0.017f);
        const uint8_t* payload=store.mapped_base()+gguf.data_offset;
        auto TP=[&](const GgufTensorInfo*t){return tensor_ptr(store,gguf,t);};
        auto build_ref=[&](uint32_t l,const std::vector<float>&input){
            const LT&z=lt.at(l);P8GLayerRef r;r.v_type=z.vw->ggml_type;r.down_type=z.dw->ggml_type;
            r.n1=rmsnorm_cpu(input,reinterpret_cast<const float*>(TP(z.an)),SEQ,H,eps);
            r.q=matmul_q4_cpu(TP(z.qw),H,H,SEQ,r.n1,reinterpret_cast<const float*>(TP(z.qb)));
            r.k=matmul_q4_cpu(TP(z.kw),H,KV,SEQ,r.n1,reinterpret_cast<const float*>(TP(z.kb)));
            r.v=p8g_matmul(z.vw,TP(z.vw),H,KV,SEQ,r.n1,reinterpret_cast<const float*>(TP(z.vb)));
            r.qr=r.q;rope_cpu(r.qr,SEQ,QH,HD,0,theta);
            r.kr=r.k;rope_cpu(r.kr,SEQ,KVH,HD,0,theta);
            r.attn=attention_cpu(r.qr,r.kr,r.v,SEQ,QH,KVH,HD);
            r.o=matmul_q4_cpu(TP(z.ow),H,H,SEQ,r.attn,nullptr);
            r.r1=add_cpu(input,r.o);
            r.n2=rmsnorm_cpu(r.r1,reinterpret_cast<const float*>(TP(z.fn)),SEQ,H,eps);
            r.g=matmul_q4_cpu(TP(z.gw),H,FFN,SEQ,r.n2,nullptr);
            r.u=matmul_q4_cpu(TP(z.uw),H,FFN,SEQ,r.n2,nullptr);
            r.s=swiglu_cpu(r.g,r.u);
            r.d=p8g_matmul(z.dw,TP(z.dw),FFN,H,SEQ,r.s,nullptr);
            r.out=add_cpu(r.r1,r.d);
            return r;
        };
        std::array<P8GLayerRef,EXEC_LAYERS> ref;
        ref[0]=build_ref(0u,x);
        ref[1]=build_ref(1u,ref[0].out);
        ref[2]=build_ref(2u,ref[1].out);
        ref[3]=build_ref(3u,ref[2].out);
        const std::vector<float>&X_CPU=ref[3].s;
        const std::vector<float>&Y_CC=ref[3].d;

        VkRuntime vk;vk.init();std::vector<Buffer> arenas;arenas.reserve(19);
        for(const auto&a:arenas_plan)arenas.push_back(vk.make_buffer(a.end-a.start,payload+a.start));
        auto BIND=[&](const std::string&n)->const P8GBinding&{return bindings.at(n);};
        auto AB=[&](const std::string&n)->Buffer*{const auto&b=BIND(n);return &arenas.at(b.slices[0].arena);};
        auto BASE=[&](const std::string&n)->uint32_t{uint64_t x0=BIND(n).slices[0].arena_byte_base;if(x0>0xffffffffull)throw std::runtime_error("P8-G1 base exceeds uint32");return uint32_t(x0);};
        auto FBASE=[&](const std::string&n)->uint32_t{uint32_t b=BASE(n);if(b%4u)throw std::runtime_error("P8-G1 unaligned F32 binding");return b/4u;};
        float zero=0.0f;auto fb=[&](uint64_t n){return vk.make_buffer(n*sizeof(float));};
        Buffer b_x=vk.make_buffer(x.size()*sizeof(float),x.data()),b_dummy=vk.make_buffer(sizeof(float),&zero);
        Buffer b_kcache=fb(uint64_t(EXEC_LAYERS)*MAXCTX*KV),b_vcache=fb(uint64_t(EXEC_LAYERS)*MAXCTX*KV);
        std::array<P8GLayerBuffers,EXEC_LAYERS> gpu;
        auto alloc_layer=[&](P8GLayerBuffers&b){
            b.n1=fb(uint64_t(SEQ)*H);b.q=fb(uint64_t(SEQ)*H);b.k=fb(uint64_t(SEQ)*KV);b.v=fb(uint64_t(SEQ)*KV);
            b.qr=fb(uint64_t(SEQ)*H);b.kr=fb(uint64_t(SEQ)*KV);b.attn=fb(uint64_t(SEQ)*H);b.o=fb(uint64_t(SEQ)*H);
            b.r1=fb(uint64_t(SEQ)*H);b.n2=fb(uint64_t(SEQ)*H);b.g=fb(uint64_t(SEQ)*FFN);b.u=fb(uint64_t(SEQ)*FFN);
            b.s=fb(uint64_t(SEQ)*FFN);b.d=fb(uint64_t(SEQ)*H);b.out=fb(uint64_t(SEQ)*H);
        };
        for(auto&b:gpu)alloc_layer(b);

        struct PCRms{uint32_t n,batch;float eps;uint32_t w_base;};
        struct PCGemm{uint32_t n,rows,batch,row_bytes,add_bias,w_base_bytes,bias_base;};
        struct PCFusedGU{uint32_t n,rows,batch,row_bytes,gate_base_bytes,up_base_bytes;};
        struct PCRope{uint32_t batch,heads,head_dim,pos_base;float theta;};
        struct PCKV{uint32_t layer,cache_pos_start,batch,max_ctx,kv_dim;};
        struct PCAttn{uint32_t q_heads,kv_heads,seq,dim;float scale;};
        struct PCN{uint32_t n;};

        std::vector<DispatchOp> prefix_ops;
        auto add_prefix=[&](const std::string&name,const std::string&spv,std::vector<Buffer*>bufs,std::vector<uint8_t>push,uint32_t gx,uint32_t gy=1){
            prefix_ops.push_back({name,join_path_p8c(shader_dir,spv),std::move(bufs),std::move(push),gx,gy,1});
        };
        auto append_prefix_layer=[&](uint32_t l,Buffer*input,P8GLayerBuffers&b,bool include_down_and_residual){
            const LT&z=lt.at(l);std::string p="L"+std::to_string(l)+".";
            add_prefix(p+"attn_rmsnorm","p7_rmsnorm_seq.spv",{input,AB(z.AN),&b.n1},push_bytes(PCRms{H,SEQ,eps,FBASE(z.AN)}),SEQ);
            add_prefix(p+"q_proj","p7c_ffn_q4k_tiled.spv",{AB(z.QW),&b.n1,AB(z.QB),&b.q},push_bytes(PCGemm{H,H,SEQ,q4_row_bytes(H),1,BASE(z.QW),FBASE(z.QB)}),(H+7u)/8u,(SEQ+7u)/8u);
            add_prefix(p+"k_proj","p7c_ffn_q4k_tiled.spv",{AB(z.KW),&b.n1,AB(z.KB),&b.k},push_bytes(PCGemm{H,KV,SEQ,q4_row_bytes(H),1,BASE(z.KW),FBASE(z.KB)}),(KV+7u)/8u,(SEQ+7u)/8u);
            uint32_t vrb=z.vw->ggml_type==Q4?q4_row_bytes(H):q6_row_bytes(H);
            add_prefix(p+"v_proj",z.vw->ggml_type==Q4?"p7c_ffn_q4k_tiled.spv":"p7c_ffn_q6k_tiled.spv",{AB(z.VW),&b.n1,AB(z.VB),&b.v},push_bytes(PCGemm{H,KV,SEQ,vrb,1,BASE(z.VW),FBASE(z.VB)}),(KV+7u)/8u,(SEQ+7u)/8u);
            add_prefix(p+"q_rope","p7_rope_seq.spv",{&b.q,&b.qr},push_bytes(PCRope{SEQ,QH,HD,0,theta}),(SEQ*QH*(HD/2u)+127u)/128u);
            add_prefix(p+"k_rope","p7_rope_seq.spv",{&b.k,&b.kr},push_bytes(PCRope{SEQ,KVH,HD,0,theta}),(SEQ*KVH*(HD/2u)+127u)/128u);
            add_prefix(p+"kv_store","p7_kv_store.spv",{&b.kr,&b.v,&b_kcache,&b_vcache},push_bytes(PCKV{l,0,SEQ,MAXCTX,KV}),(SEQ*KV+255u)/256u);
            add_prefix(p+"causal_gqa","p7_attention_prefill_online.spv",{&b.qr,&b.kr,&b.v,&b.attn},push_bytes(PCAttn{QH,KVH,SEQ,HD,1.0f/std::sqrt(float(HD))}),SEQ*QH);
            add_prefix(p+"o_proj","p7c_ffn_q4k_tiled.spv",{AB(z.OW),&b.attn,&b_dummy,&b.o},push_bytes(PCGemm{H,H,SEQ,q4_row_bytes(H),0,BASE(z.OW),0}),(H+7u)/8u,(SEQ+7u)/8u);
            add_prefix(p+"attn_residual","p7_add.spv",{input,&b.o,&b.r1},push_bytes(PCN{SEQ*H}),(SEQ*H+255u)/256u);
            add_prefix(p+"ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b.r1,AB(z.FN),&b.n2},push_bytes(PCRms{H,SEQ,eps,FBASE(z.FN)}),SEQ);
            add_prefix(p+"ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv",{AB(z.GW),AB(z.UW),&b.n2,&b.g,&b.u},push_bytes(PCFusedGU{H,FFN,SEQ,q4_row_bytes(H),BASE(z.GW),BASE(z.UW)}),(FFN+7u)/8u,(SEQ+15u)/16u);
            add_prefix(p+"swiglu","p7_swiglu.spv",{&b.g,&b.u,&b.s},push_bytes(PCN{SEQ*FFN}),(SEQ*FFN+255u)/256u);
            if(include_down_and_residual){
                uint32_t drb=z.dw->ggml_type==Q4?q4_row_bytes(FFN):q6_row_bytes(FFN);
                add_prefix(p+"ffn_down",z.dw->ggml_type==Q4?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv",{AB(z.DW),&b.s,&b_dummy,&b.d},push_bytes(PCGemm{FFN,H,SEQ,drb,0,BASE(z.DW),0}),(H+7u)/8u,(SEQ+15u)/16u);
                add_prefix(p+"ffn_residual","p7_add.spv",{&b.r1,&b.d,&b.out},push_bytes(PCN{SEQ*H}),(SEQ*H+255u)/256u);
            }
        };

        append_prefix_layer(0u,&b_x,gpu[0],true);
        Buffer* layer1_input=&gpu[0].out;
        append_prefix_layer(1u,layer1_input,gpu[1],true);
        Buffer* layer2_input=&gpu[1].out;
        append_prefix_layer(2u,layer2_input,gpu[2],true);
        Buffer* layer3_input=&gpu[2].out;
        append_prefix_layer(3u,layer3_input,gpu[3],false);
        const bool direct_gpu_handoff=(layer1_input==&gpu[0].out)&&(layer2_input==&gpu[1].out)&&(layer3_input==&gpu[2].out);
        if(prefix_ops.size()!=58u)throw std::runtime_error("P8-G1 prefix operation census mismatch");

        PreparedChain prefix_chain=vk.prepare_chain(prefix_ops);
        ChainStats prefix_stats=vk.execute_prepared(prefix_chain,prefix_ops);
        auto readf=[&](Buffer&b,size_t n){std::vector<float>v(n);std::memcpy(v.data(),b.mapped,n*sizeof(float));return v;};
        const std::vector<float> X_GPU=readf(gpu[3].s,X_CPU.size());
        const Metrics x_metrics=compare_vec(X_CPU,X_GPU,0.02,0.005);
        const bool x_finite=p8g_finite(X_GPU);
        const bool x_parent_reproduced=
            std::abs(x_metrics.max_abs-0.00838851928711)<=1e-6&&
            std::abs(x_metrics.rmse-0.0000710637175468)<=1e-7;

        const std::vector<float> Y_CG=matmul_q4_cpu(TP(lt[3].dw),FFN,H,SEQ,X_GPU,nullptr);

        Buffer b_x_cpu=vk.make_buffer(X_CPU.size()*sizeof(float),X_CPU.data());
        Buffer b_y_gc_p7g=fb(uint64_t(SEQ)*H),b_y_gg_p7g=fb(uint64_t(SEQ)*H);
        Buffer b_y_gc_p7c=fb(uint64_t(SEQ)*H),b_y_gg_p7c=fb(uint64_t(SEQ)*H);

        std::vector<DispatchOp> diag_ops;
        auto add_diag=[&](const std::string&name,const std::string&spv,Buffer*in,Buffer*outb,uint32_t gy){
            diag_ops.push_back({name,join_path_p8c(shader_dir,spv),{AB(lt[3].DW),in,&b_dummy,outb},
                                push_bytes(PCGemm{FFN,H,SEQ,q4_row_bytes(FFN),0,BASE(lt[3].DW),0}),
                                (H+7u)/8u,gy,1});
        };
        add_diag("Y_GC_P7G","p7g_ffn_q4k_tiled16.spv",&b_x_cpu,&b_y_gc_p7g,(SEQ+15u)/16u);
        add_diag("Y_GG_P7G","p7g_ffn_q4k_tiled16.spv",&gpu[3].s,&b_y_gg_p7g,(SEQ+15u)/16u);
        add_diag("Y_GC_P7C","p7c_ffn_q4k_tiled.spv",&b_x_cpu,&b_y_gc_p7c,(SEQ+7u)/8u);
        add_diag("Y_GG_P7C","p7c_ffn_q4k_tiled.spv",&gpu[3].s,&b_y_gg_p7c,(SEQ+7u)/8u);
        if(diag_ops.size()!=4u)throw std::runtime_error("P8-G1 diagnostic operation census mismatch");

        PreparedChain diag_chain=vk.prepare_chain(diag_ops);
        ChainStats diag_stats=vk.execute_prepared(diag_chain,diag_ops);
        const auto Y_GC_P7G=readf(b_y_gc_p7g,Y_CC.size());
        const auto Y_GG_P7G=readf(b_y_gg_p7g,Y_CC.size());
        const auto Y_GC_P7C=readf(b_y_gc_p7c,Y_CC.size());
        const auto Y_GG_P7C=readf(b_y_gg_p7c,Y_CC.size());

        struct Obs{const char*name;Metrics m;bool finite;};
        auto obs=[&](const char*n,const std::vector<float>&a,const std::vector<float>&b){return Obs{n,compare_vec(a,b,0.02,0.005),p8g_finite(a)&&p8g_finite(b)};};
        Obs C0=obs("C0_parent_reproduction",Y_GG_P7G,Y_CC);
        Obs C1=obs("C1_p7g_cpu_input",Y_GC_P7G,Y_CC);
        Obs C2=obs("C2_p7c_cpu_input",Y_GC_P7C,Y_CC);
        Obs C3=obs("C3_cpu_input_amplification",Y_CG,Y_CC);
        Obs C4=obs("C4_p7g_gpu_input",Y_GG_P7G,Y_CG);
        Obs C5=obs("C5_p7c_gpu_input",Y_GG_P7C,Y_CG);
        Obs C6a=obs("C6a_p7g_vs_p7c_cpu_input",Y_GC_P7G,Y_GC_P7C);
        Obs C6b=obs("C6b_p7g_vs_p7c_gpu_input",Y_GG_P7G,Y_GG_P7C);
        std::array<Obs,8> comparisons={C0,C1,C2,C3,C4,C5,C6a,C6b};

        const bool c0_reproduced=(!C0.m.pass)&&C0.finite&&
            std::abs(C0.m.max_abs-0.0283279418945)<=1e-6&&
            std::abs(C0.m.rmse-0.000364843778882)<=1e-7;
        const bool parent_reproduction_pass=x_parent_reproduced&&c0_reproduced&&direct_gpu_handoff&&
            prefix_stats.dispatch_count==58u&&prefix_stats.submit_count==1u&&
            diag_stats.dispatch_count==4u&&diag_stats.submit_count==1u;

        std::string adjudication="INCONCLUSIVE";
        if(parent_reproduction_pass){
            if(!C1.m.pass&&C2.m.pass)adjudication="H-KERNEL";
            else if(!C1.m.pass&&!C2.m.pass)adjudication="H-COMMON-Q4";
            else if(C1.m.pass&&C2.m.pass&&(C4.m.pass!=C5.m.pass))adjudication="INPUT_DEPENDENT_TILED_KERNEL_SENSITIVITY";
            else if(C1.m.pass&&C2.m.pass&&!C3.m.pass&&!C0.m.pass)adjudication="H-AMPLIFICATION";
            else if(C1.m.pass&&C2.m.pass&&C3.m.pass&&!C0.m.pass&&C4.m.pass&&C5.m.pass)adjudication="H-INTERACTION";
        }else adjudication="PARENT_REPRODUCTION_FAILED";

        const bool diagnostic_valid=parent_reproduction_pass&&focus_span_exact&&x_finite&&
            piece_count==341u&&multi_count==2u&&span&&coverage;
        const std::string status=diagnostic_valid?"COMPLETE":"INVALID";

        std::ofstream o(out,std::ios::binary);if(!o)throw std::runtime_error("cannot write P8-G1 result JSON");
        o<<"{\n  \"schema\":\"arcllm.p8g1.causal_decomposition.v1\",\n";
        o<<"  \"status\":\""<<status<<"\",\n";
        o<<"  \"p8g_verdict_frozen\":\"FAIL\",\n";
        o<<"  \"focus\":{\"tensor\":\"blk.3.ffn_down.weight\",\"format\":\"Q4_K\",\"n\":18944,\"rows\":3584,\"batch\":4,\"single_piece\":true,\"source_span_exact\":"<<json_bool(focus_span_exact)<<"},\n";
        o<<"  \"binding\":{\"arena_count\":"<<arenas_plan.size()<<",\"physical_piece_count\":"<<piece_count<<",\"segmented_logical_tensor_count\":"<<multi_count<<",\"span_equivalence_pass\":"<<json_bool(span)<<",\"global_coverage_pass\":"<<json_bool(coverage)<<"},\n";
        o<<"  \"controlled_inputs\":{\"x_cpu_values\":"<<X_CPU.size()<<",\"x_gpu_values\":"<<X_GPU.size()<<",\"x_gpu_vs_x_cpu\":{\"max_abs\":"<<std::setprecision(12)<<x_metrics.max_abs<<",\"rmse\":"<<x_metrics.rmse<<",\"finite_pass\":"<<json_bool(x_finite)<<",\"frozen_gate_pass\":"<<json_bool(x_metrics.pass&&x_finite)<<",\"parent_metric_reproduced\":"<<json_bool(x_parent_reproduced)<<"}},\n";
        o<<"  \"outputs\":[\"Y_CC\",\"Y_CG\",\"Y_GC_P7G\",\"Y_GG_P7G\",\"Y_GC_P7C\",\"Y_GG_P7C\"],\n";
        o<<"  \"comparisons\":{\n";
        for(size_t i=0;i<comparisons.size();++i){const auto&c=comparisons[i];o<<"    \""<<c.name<<"\":{\"max_abs\":"<<std::setprecision(12)<<c.m.max_abs<<",\"rmse\":"<<c.m.rmse<<",\"finite_pass\":"<<json_bool(c.finite)<<",\"frozen_gate_pass\":"<<json_bool(c.m.pass&&c.finite)<<"}"<<(i+1<comparisons.size()?",":"")<<"\n";}
        o<<"  },\n";
        o<<"  \"execution\":{\"prefix_dispatches\":"<<prefix_stats.dispatch_count<<",\"prefix_submits\":"<<prefix_stats.submit_count<<",\"diagnostic_dispatches\":"<<diag_stats.dispatch_count<<",\"diagnostic_submits\":"<<diag_stats.submit_count<<",\"direct_gpu_handoff\":"<<json_bool(direct_gpu_handoff)<<",\"layer4_plus_executed\":false},\n";
        o<<"  \"reproduction\":{\"x_parent_metric_reproduced\":"<<json_bool(x_parent_reproduced)<<",\"c0_parent_failure_reproduced\":"<<json_bool(c0_reproduced)<<",\"parent_reproduction_pass\":"<<json_bool(parent_reproduction_pass)<<"},\n";
        o<<"  \"adjudication\":{\"classification\":\""<<adjudication<<"\",\"p8g_verdict_changed\":false,\"p8h_permitted\":false,\"full_inference_permitted\":false},\n";
        o<<"  \"diagnostic_valid\":"<<json_bool(diagnostic_valid)<<"\n}\n";o.close();

        std::cout<<"ArcLLM P8-G1 L3 FFN-down causal decomposition\n";
        std::cout<<"prefix dispatches="<<prefix_stats.dispatch_count<<" submits="<<prefix_stats.submit_count
                 <<" diagnostic dispatches="<<diag_stats.dispatch_count<<" submits="<<diag_stats.submit_count<<"\n";
        std::cout<<"X_GPU_vs_X_CPU max_abs="<<x_metrics.max_abs<<" rmse="<<x_metrics.rmse<<" reproduced="<<x_parent_reproduced<<"\n";
        for(const auto&c:comparisons)std::cout<<c.name<<" max_abs="<<c.m.max_abs<<" rmse="<<c.m.rmse<<" pass="<<(c.m.pass&&c.finite)<<"\n";
        std::cout<<"P8-G1 "<<status<<" classification="<<adjudication<<" parent_reproduced="<<parent_reproduction_pass<<"\n";

        vk.destroy_prepared(diag_chain);vk.destroy_prepared(prefix_chain);
        vk.destroy_buffer(b_y_gg_p7c);vk.destroy_buffer(b_y_gc_p7c);vk.destroy_buffer(b_y_gg_p7g);vk.destroy_buffer(b_y_gc_p7g);vk.destroy_buffer(b_x_cpu);
        auto destroy_layer=[&](P8GLayerBuffers&b){for(Buffer*p:{&b.out,&b.d,&b.s,&b.u,&b.g,&b.n2,&b.r1,&b.o,&b.attn,&b.kr,&b.qr,&b.v,&b.k,&b.q,&b.n1})vk.destroy_buffer(*p);};
        for(int l=int(EXEC_LAYERS)-1;l>=0;--l)destroy_layer(gpu[size_t(l)]);
        vk.destroy_buffer(b_vcache);vk.destroy_buffer(b_kcache);vk.destroy_buffer(b_dummy);vk.destroy_buffer(b_x);
        for(auto&b:arenas)vk.destroy_buffer(b);
        return diagnostic_valid?0:21;
    }catch(const std::exception&e){
        std::ofstream o(out,std::ios::binary);if(o)o<<"{\n  \"schema\":\"arcllm.p8g1.causal_decomposition.v1\",\n  \"status\":\"ERROR\",\n  \"error\":\""<<p8g_escape(e.what())<<"\"\n}\n";
        std::cerr<<e.what()<<"\n";return 2;
    }
}
