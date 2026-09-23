#define main p8c_main_disabled
#include "arcllm_v1_i001r_p8c_profile_runtime.cpp"
#undef main

#include <map>
#include <set>
#include <array>
#include <limits>
#include <thread>

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




static uint64_t q2_fnv1a64(const void* data,size_t bytes){
    const uint8_t*p=reinterpret_cast<const uint8_t*>(data);
    uint64_t h=1469598103934665603ull;
    for(size_t i=0;i<bytes;++i){h^=uint64_t(p[i]);h*=1099511628211ull;}
    return h;
}
static std::string q2_hex64(uint64_t v){
    std::ostringstream ss;ss<<std::hex<<std::setw(16)<<std::setfill('0')<<v;return ss.str();
}
struct Q2Top2{
    uint32_t top1=0,top2=0;
    float logit1=-std::numeric_limits<float>::infinity();
    float logit2=-std::numeric_limits<float>::infinity();
    bool finite=true;
};
static Q2Top2 q2_top2(const float*p,uint32_t n){
    Q2Top2 r;
    for(uint32_t i=0;i<n;++i){
        const float v=p[i];
        if(!std::isfinite(v)){r.finite=false;continue;}
        if(v>r.logit1){r.logit2=r.logit1;r.top2=r.top1;r.logit1=v;r.top1=i;}
        else if(v>r.logit2){r.logit2=v;r.top2=i;}
    }
    if(!std::isfinite(r.logit1)||!std::isfinite(r.logit2))r.finite=false;
    return r;
}
struct Q1StepObs{
    std::string stage;
    uint32_t input_position=0;
    uint32_t predicted_position=0;
    uint32_t input_token=0;
    uint32_t top1=0,top2=0;
    double top1_logit=0,top2_logit=0,margin=0;
    std::string logits_hash,hidden_hash;
    bool logits_finite=false,argmax_valid=false,feedback_valid=true;
    uint32_t dispatches=0,submits=0;
    double wall_ms=0;
};
struct Q1ExecutionObs{
    std::string id;
    std::vector<uint32_t> generated;
    std::vector<Q1StepObs> steps;
    bool f0=false,f1=false;
    bool dispatch_census_pass=false;
    bool finite_stage_pass=false;
    bool positions_pass=false;
    bool full_logits_pass=false;
    double wall_ms=0;
};

int main(int argc,char**argv){
    std::string model,shader_dir,implementation_commit,workload,session,out="i002_t1_transfer.json";
    int warmups=1,measured=5;
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            auto need=[&](const char*f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};
            if(a=="--model")model=need("--model");
            else if(a=="--shader-dir")shader_dir=need("--shader-dir");
            else if(a=="--implementation-commit")implementation_commit=need("--implementation-commit");
            else if(a=="--workload")workload=need("--workload");
            else if(a=="--session")session=need("--session");
            else if(a=="--warmups")warmups=std::stoi(need("--warmups"));
            else if(a=="--measured")measured=std::stoi(need("--measured"));
            else if(a=="--out")out=need("--out");
        }
        if(model.empty()||shader_dir.empty()||implementation_commit.empty()||workload.empty()||session.empty())
            throw std::runtime_error("I002 T1 required arguments missing");
        if(session!="A"&&session!="B")throw std::runtime_error("I002 T1 session must be A or B");
        if(warmups!=1||measured!=5)throw std::runtime_error("Q2 frozen repetition count mismatch");
        auto setup_t0=std::chrono::steady_clock::now();
        GgufInfo gguf=GgufReader(model).read();
        TensorStore store;TensorStoreReport ts=store.inspect(model,gguf);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("Q2 TensorStore invariants failed");

        const uint32_t H=3584,QH=28,KVH=4,HD=128,KV=512,FFN=18944,MAXSEQ=256,MAXCTX=4096,VOC=152064;
        const uint32_t LAYERS=28,F32=0,Q4=12,Q6=14;
        const uint32_t EMB_BOUNDARY=133152,OUT_BOUNDARY=91304,EMB_RB=2016,OUT_RB=2940,LM_CHUNK=8192;
        const uint32_t LM_DISPATCHES=(VOC+LM_CHUNK-1u)/LM_CHUNK;
        const uint32_t EXPECT_PREFILL=441,EXPECT_DECODE=469;
        const uint64_t P8B_WEIGHT_BYTES=4677120000ull,P8B_KV_BYTES=469762048ull,P8B_WORKING_BYTES=200888324ull;
        const uint64_t P8B_TOTAL_RESIDENT=5347770372ull,P8_USABLE_BUDGET=16374562816ull;

        if(gguf.tensor_count!=339u||gguf.tensors.size()!=339u)throw std::runtime_error("Q2 exact tensor census mismatch");
        auto bc=gguf.scalars.find("qwen2.block_count");
        if(bc==gguf.scalars.end()||std::stoul(bc->second)!=LAYERS)throw std::runtime_error("Q2 block count mismatch");

        auto arenas_plan=p8g_recompute_arenas(gguf);
        if(!p8g_arena_equal(arenas_plan,p8g_expected_arenas()))throw std::runtime_error("Q2 frozen arena mismatch");
        uint32_t piece_count=0,multi_count=0;bool span=false,coverage=false;
        auto bindings=p8g_build_bindings(gguf,arenas_plan,piece_count,multi_count,span,coverage);
        if(bindings.size()!=339u||piece_count!=341u||multi_count!=2u||!span||!coverage)
            throw std::runtime_error("Q2 graph binding regression");

        float eps=1e-6f;auto ei=gguf.scalars.find("qwen2.attention.layer_norm_rms_epsilon");
        if(ei!=gguf.scalars.end())eps=std::stof(ei->second);
        float theta=1000000.0f;auto ri=gguf.scalars.find("qwen2.rope.freq_base");
        if(ri!=gguf.scalars.end())theta=std::stof(ri->second);

        auto nm=[](uint32_t l,const char*s){return std::string("blk.")+std::to_string(l)+s;};
        struct LT{
            std::string AN,QW,KW,VW,QB,KB,VB,OW,FN,GW,UW,DW;
            const GgufTensorInfo *an=nullptr,*qw=nullptr,*kw=nullptr,*vw=nullptr,*qb=nullptr,*kb=nullptr,*vb=nullptr,*ow=nullptr,*fn=nullptr,*gw=nullptr,*uw=nullptr,*dw=nullptr;
        };
        std::array<LT,LAYERS> lt;
        for(uint32_t l=0;l<LAYERS;++l){
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
            if((z.vw->ggml_type!=Q4&&z.vw->ggml_type!=Q6)||(z.dw->ggml_type!=Q4&&z.dw->ggml_type!=Q6))
                throw std::runtime_error("Q2 mixed-quant layer obstruction: "+std::to_string(l));
            for(const auto&n:std::vector<std::string>{z.AN,z.QW,z.KW,z.VW,z.QB,z.KB,z.VB,z.OW,z.FN,z.GW,z.UW,z.DW})
                if(bindings.at(n).slices.size()!=1u)throw std::runtime_error("Q2 decoder tensor unexpectedly segmented: "+n);
            lt[l]=std::move(z);
        }

        const auto*emb=find_tensor(gguf,"token_embd.weight");
        const auto*outw=find_tensor(gguf,"output.weight");
        const auto*outn=find_tensor(gguf,"output_norm.weight");
        require_dims(emb,Q4,H,VOC,"token_embd.weight");require_dims(outw,Q6,H,VOC,"output.weight");require_vec(outn,F32,H,"output_norm.weight");
        const auto&EB=bindings.at("token_embd.weight");const auto&OB=bindings.at("output.weight");
        if(EB.slices.size()!=2u||OB.slices.size()!=2u||bindings.at("output_norm.weight").slices.size()!=1u)
            throw std::runtime_error("Q2 endpoint segmentation mismatch");
        if(EB.slices[0].row_count!=EMB_BOUNDARY||EB.slices[1].row_start!=EMB_BOUNDARY||
           OB.slices[0].row_count!=OUT_BOUNDARY||OB.slices[1].row_start!=OUT_BOUNDARY||
           EB.row_bytes!=EMB_RB||OB.row_bytes!=OUT_RB)
            throw std::runtime_error("Q2 endpoint segmented geometry mismatch");
        for(const auto&s:EB.slices)if(s.arena_byte_base!=0u)throw std::runtime_error("Q2 embedding segment does not start at arena base");
        for(const auto&s:OB.slices)if(s.arena_byte_base!=0u)throw std::runtime_error("Q2 output segment does not start at arena base");

        const uint8_t*payload=store.mapped_base()+gguf.data_offset;
        VkRuntime vk;vk.init();
        std::vector<Buffer> arenas;arenas.reserve(19);
        uint64_t weight_requested=0;
        for(const auto&a:arenas_plan){uint64_t bytes=a.end-a.start;weight_requested+=bytes;arenas.push_back(vk.make_buffer(bytes,payload+a.start));}
        if(weight_requested!=P8B_WEIGHT_BYTES)throw std::runtime_error("Q2 weight residency bytes mismatch");

        auto BIND=[&](const std::string&n)->const P8GBinding&{return bindings.at(n);};
        auto AB=[&](const std::string&n)->Buffer*{const auto&b=BIND(n);if(b.slices.size()!=1u)throw std::runtime_error("Q2 AB on segmented tensor: "+n);return &arenas.at(b.slices[0].arena);};
        auto BASE=[&](const std::string&n)->uint32_t{const auto&b=BIND(n);if(b.slices.size()!=1u)throw std::runtime_error("Q2 BASE on segmented tensor: "+n);uint64_t x=b.slices[0].arena_byte_base;if(x>0xffffffffull)throw std::runtime_error("Q2 base exceeds uint32");return uint32_t(x);};
        auto FBASE=[&](const std::string&n)->uint32_t{uint32_t b=BASE(n);if(b%4u)throw std::runtime_error("Q2 unaligned F32 binding");return b/4u;};

        uint64_t working_requested=0,kv_requested=0;
        auto wb=[&](uint64_t bytes,const void*init){working_requested+=bytes;return vk.make_buffer(bytes,init);};
        auto wf=[&](uint64_t elems){return wb(elems*sizeof(float),nullptr);};
        auto kf=[&](uint64_t elems){uint64_t bytes=elems*sizeof(float);kv_requested+=bytes;return vk.make_buffer(bytes);};

        std::vector<uint32_t> input_ids;
        if(workload=="W-S"){
            input_ids={1u,133151u,133152u,152062u};
        }else if(workload=="W-C"){
            input_ids.resize(256u);
            input_ids[0]=1u;input_ids[1]=133151u;input_ids[2]=133152u;input_ids[3]=152062u;
            for(uint32_t i=4u;i<256u;++i)input_ids[i]=1u+((104729u+7919u*i)%152063u);
        }else throw std::runtime_error("Q2 workload must be W-S or W-C");
        const uint32_t seq=uint32_t(input_ids.size());
        if(seq!=4u&&seq!=256u)throw std::runtime_error("Q2 frozen prompt length mismatch");
        for(uint32_t id:input_ids)if(id>=VOC)throw std::runtime_error("Q2 frozen token out of vocabulary");
        const std::string prompt_hash=q2_hex64(q2_fnv1a64(input_ids.data(),input_ids.size()*sizeof(uint32_t)));

        uint32_t decode_id=0;float zero=0.0f;
        Buffer b_ids=wb(uint64_t(MAXSEQ)*sizeof(uint32_t),nullptr),b_dec_id=wb(sizeof(uint32_t),&decode_id),b_dummy=wb(sizeof(float),&zero);
        Buffer b_h0=wf(uint64_t(MAXSEQ)*H),b_h1=wf(uint64_t(MAXSEQ)*H),b_n1=wf(uint64_t(MAXSEQ)*H);
        Buffer b_q=wf(uint64_t(MAXSEQ)*H),b_k=wf(uint64_t(MAXSEQ)*KV),b_v=wf(uint64_t(MAXSEQ)*KV);
        Buffer b_qr=wf(uint64_t(MAXSEQ)*H),b_kr=wf(uint64_t(MAXSEQ)*KV),b_attn=wf(uint64_t(MAXSEQ)*H);
        Buffer b_o=wf(uint64_t(MAXSEQ)*H),b_r1=wf(uint64_t(MAXSEQ)*H),b_n2=wf(uint64_t(MAXSEQ)*H);
        Buffer b_g=wf(uint64_t(MAXSEQ)*FFN),b_u=wf(uint64_t(MAXSEQ)*FFN),b_s=wf(uint64_t(MAXSEQ)*FFN);
        Buffer b_d=wf(uint64_t(MAXSEQ)*H),b_norm=wf(uint64_t(MAXSEQ)*H),b_logits=wf(VOC);
        const uint64_t cache_elems=uint64_t(LAYERS)*MAXCTX*KV;
        Buffer b_kcache=kf(cache_elems),b_vcache=kf(cache_elems);
        if(kv_requested!=P8B_KV_BYTES)throw std::runtime_error("Q2 KV residency bytes mismatch");
        const uint64_t total_requested=weight_requested+kv_requested+working_requested;
        if(working_requested>P8B_WORKING_BYTES||total_requested>P8B_TOTAL_RESIDENT||total_requested>P8_USABLE_BUDGET)
            throw std::runtime_error("Q2 requested residency exceeds frozen P8-B envelope");

        const VkMemoryPropertyFlags required_mem=
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        const bool memory_flags_pass=(vk.memory_type_flags()&required_mem)==required_mem;

        struct PCEmbSeg{uint32_t n,count,row_bytes,boundary;};
        struct PCRms{uint32_t n,batch;float eps;uint32_t w_base;};
        struct PCGemm{uint32_t n,rows,batch,row_bytes,add_bias,w_base_bytes,bias_base;};
        struct PCFusedGU{uint32_t n,rows,batch,row_bytes,gate_base_bytes,up_base_bytes;};
        struct PCRope{uint32_t batch,heads,head_dim,pos_base;float theta;};
        struct PCKV{uint32_t layer,cache_pos_start,batch,max_ctx,kv_dim;};
        struct PCAttnPrefill{uint32_t q_heads,kv_heads,seq,dim;float scale;};
        struct PCAttnKV{uint32_t layer,pos,max_ctx,q_heads,kv_heads,dim;float scale;};
        struct PCN{uint32_t n;};
        struct PCLMSeg{uint32_t n,row_bytes,row_start,row_count,boundary,x_base;};

        auto addop=[&](std::vector<DispatchOp>&ops,const std::string&name,const std::string&sh,std::vector<Buffer*>bufs,std::vector<uint8_t>push,uint32_t gx,uint32_t gy=1,uint32_t gz=1){
            ops.push_back({name,join_path_p8c(shader_dir,sh),std::move(bufs),std::move(push),gx,gy,gz});
        };
        Buffer*emb0=&arenas.at(EB.slices[0].arena);Buffer*emb1=&arenas.at(EB.slices[1].arena);
        Buffer*out0=&arenas.at(OB.slices[0].arena);Buffer*out1=&arenas.at(OB.slices[1].arena);

        auto append_lm=[&](std::vector<DispatchOp>&ops,uint32_t x_base){
            for(uint32_t rs=0;rs<VOC;rs+=LM_CHUNK){
                uint32_t rc=(std::min)(LM_CHUNK,VOC-rs);
                addop(ops,"lm_head","p8q1_lmhead_q6k_segmented_chunk.spv",{out0,out1,&b_norm,&b_logits},
                      push_bytes(PCLMSeg{H,OUT_RB,rs,rc,OUT_BOUNDARY,x_base}),(rc+63u)/64u);
            }
        };

        auto build_prefill=[&](uint32_t seq){
            std::vector<DispatchOp>ops;
            addop(ops,"token_embedding","p8c_embedding_q4k_segmented_probe.spv",{emb0,emb1,&b_ids,&b_h0},
                  push_bytes(PCEmbSeg{H,seq,EMB_RB,EMB_BOUNDARY}),(seq*H+255u)/256u);
            Buffer*cur=&b_h0;Buffer*nxt=&b_h1;
            for(uint32_t l=0;l<LAYERS;++l){
                const LT&z=lt[l];std::string p="L"+(l<10?std::string("0"):std::string())+std::to_string(l)+".";
                addop(ops,p+"attn_rmsnorm","p7_rmsnorm_seq.spv",{cur,AB(z.AN),&b_n1},push_bytes(PCRms{H,seq,eps,FBASE(z.AN)}),seq);
                uint32_t vrb=z.vw->ggml_type==Q4?q4_row_bytes(H):q6_row_bytes(H);
                addop(ops,p+"q_proj","p7c_ffn_q4k_tiled.spv",{AB(z.QW),&b_n1,AB(z.QB),&b_q},push_bytes(PCGemm{H,H,seq,q4_row_bytes(H),1,BASE(z.QW),FBASE(z.QB)}),(H+7u)/8u,(seq+7u)/8u);
                addop(ops,p+"k_proj","p7c_ffn_q4k_tiled.spv",{AB(z.KW),&b_n1,AB(z.KB),&b_k},push_bytes(PCGemm{H,KV,seq,q4_row_bytes(H),1,BASE(z.KW),FBASE(z.KB)}),(KV+7u)/8u,(seq+7u)/8u);
                addop(ops,p+"v_proj",z.vw->ggml_type==Q4?"p7c_ffn_q4k_tiled.spv":"p7c_ffn_q6k_tiled.spv",{AB(z.VW),&b_n1,AB(z.VB),&b_v},push_bytes(PCGemm{H,KV,seq,vrb,1,BASE(z.VW),FBASE(z.VB)}),(KV+7u)/8u,(seq+7u)/8u);
                addop(ops,p+"q_rope","p7_rope_seq.spv",{&b_q,&b_qr},push_bytes(PCRope{seq,QH,HD,0,theta}),(seq*QH*(HD/2u)+127u)/128u);
                addop(ops,p+"k_rope","p7_rope_seq.spv",{&b_k,&b_kr},push_bytes(PCRope{seq,KVH,HD,0,theta}),(seq*KVH*(HD/2u)+127u)/128u);
                addop(ops,p+"kv_store","p7_kv_store.spv",{&b_kr,&b_v,&b_kcache,&b_vcache},push_bytes(PCKV{l,0,seq,MAXCTX,KV}),(seq*KV+255u)/256u);
                addop(ops,p+"causal_gqa","p7_attention_prefill_online.spv",{&b_qr,&b_kr,&b_v,&b_attn},push_bytes(PCAttnPrefill{QH,KVH,seq,HD,1.0f/std::sqrt(float(HD))}),seq*QH);
                addop(ops,p+"o_proj","p7c_ffn_q4k_tiled.spv",{AB(z.OW),&b_attn,&b_dummy,&b_o},push_bytes(PCGemm{H,H,seq,q4_row_bytes(H),0,BASE(z.OW),0}),(H+7u)/8u,(seq+7u)/8u);
                addop(ops,p+"attn_residual","p7_add.spv",{cur,&b_o,&b_r1},push_bytes(PCN{seq*H}),(seq*H+255u)/256u);
                addop(ops,p+"ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b_r1,AB(z.FN),&b_n2},push_bytes(PCRms{H,seq,eps,FBASE(z.FN)}),seq);
                addop(ops,p+"ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv",{AB(z.GW),AB(z.UW),&b_n2,&b_g,&b_u},push_bytes(PCFusedGU{H,FFN,seq,q4_row_bytes(H),BASE(z.GW),BASE(z.UW)}),(FFN+7u)/8u,(seq+15u)/16u);
                addop(ops,p+"swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(PCN{seq*FFN}),(seq*FFN+255u)/256u);
                uint32_t drb=z.dw->ggml_type==Q4?q4_row_bytes(FFN):q6_row_bytes(FFN);
                addop(ops,p+"ffn_down",z.dw->ggml_type==Q4?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv",{AB(z.DW),&b_s,&b_dummy,&b_d},push_bytes(PCGemm{FFN,H,seq,drb,0,BASE(z.DW),0}),(H+7u)/8u,(seq+15u)/16u);
                addop(ops,p+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(PCN{seq*H}),(seq*H+255u)/256u);
                std::swap(cur,nxt);
            }
            addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB("output_norm.weight"),&b_norm},push_bytes(PCRms{H,seq,eps,FBASE("output_norm.weight")}),seq);
            append_lm(ops,(seq-1u)*H);
            return ops;
        };

        auto build_decode=[&](uint32_t pos){
            std::vector<DispatchOp>ops;
            addop(ops,"token_embedding","p8c_embedding_q4k_segmented_probe.spv",{emb0,emb1,&b_dec_id,&b_h0},
                  push_bytes(PCEmbSeg{H,1,EMB_RB,EMB_BOUNDARY}),(H+255u)/256u);
            Buffer*cur=&b_h0;Buffer*nxt=&b_h1;
            for(uint32_t l=0;l<LAYERS;++l){
                const LT&z=lt[l];std::string p="L"+(l<10?std::string("0"):std::string())+std::to_string(l)+".";
                addop(ops,p+"attn_rmsnorm","p7_rmsnorm_seq.spv",{cur,AB(z.AN),&b_n1},push_bytes(PCRms{H,1,eps,FBASE(z.AN)}),1);
                uint32_t vrb=z.vw->ggml_type==Q4?q4_row_bytes(H):q6_row_bytes(H);
                addop(ops,p+"q_proj","p7_q4k_gemm_2d.spv",{AB(z.QW),&b_n1,AB(z.QB),&b_q},push_bytes(PCGemm{H,H,1,q4_row_bytes(H),1,BASE(z.QW),FBASE(z.QB)}),(H+63u)/64u);
                addop(ops,p+"k_proj","p7_q4k_gemm_2d.spv",{AB(z.KW),&b_n1,AB(z.KB),&b_k},push_bytes(PCGemm{H,KV,1,q4_row_bytes(H),1,BASE(z.KW),FBASE(z.KB)}),(KV+63u)/64u);
                addop(ops,p+"v_proj",z.vw->ggml_type==Q4?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv",{AB(z.VW),&b_n1,AB(z.VB),&b_v},push_bytes(PCGemm{H,KV,1,vrb,1,BASE(z.VW),FBASE(z.VB)}),(KV+63u)/64u);
                addop(ops,p+"q_rope","p7_rope_seq.spv",{&b_q,&b_qr},push_bytes(PCRope{1,QH,HD,pos,theta}),(QH*(HD/2u)+127u)/128u);
                addop(ops,p+"k_rope","p7_rope_seq.spv",{&b_k,&b_kr},push_bytes(PCRope{1,KVH,HD,pos,theta}),(KVH*(HD/2u)+127u)/128u);
                addop(ops,p+"kv_store","p7_kv_store.spv",{&b_kr,&b_v,&b_kcache,&b_vcache},push_bytes(PCKV{l,pos,1,MAXCTX,KV}),(KV+255u)/256u);
                addop(ops,p+"cached_gqa","p7_attention_kv_online.spv",{&b_qr,&b_kcache,&b_vcache,&b_attn},push_bytes(PCAttnKV{l,pos,MAXCTX,QH,KVH,HD,1.0f/std::sqrt(float(HD))}),QH);
                addop(ops,p+"o_proj","p7_q4k_gemm_2d.spv",{AB(z.OW),&b_attn,&b_dummy,&b_o},push_bytes(PCGemm{H,H,1,q4_row_bytes(H),0,BASE(z.OW),0}),(H+63u)/64u);
                addop(ops,p+"attn_residual","p7_add.spv",{cur,&b_o,&b_r1},push_bytes(PCN{H}),(H+255u)/256u);
                addop(ops,p+"ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b_r1,AB(z.FN),&b_n2},push_bytes(PCRms{H,1,eps,FBASE(z.FN)}),1);
                addop(ops,p+"ffn_gate","p7_q4k_gemm_2d.spv",{AB(z.GW),&b_n2,&b_dummy,&b_g},push_bytes(PCGemm{H,FFN,1,q4_row_bytes(H),0,BASE(z.GW),0}),(FFN+63u)/64u);
                addop(ops,p+"ffn_up","p7_q4k_gemm_2d.spv",{AB(z.UW),&b_n2,&b_dummy,&b_u},push_bytes(PCGemm{H,FFN,1,q4_row_bytes(H),0,BASE(z.UW),0}),(FFN+63u)/64u);
                addop(ops,p+"swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(PCN{FFN}),(FFN+255u)/256u);
                uint32_t drb=z.dw->ggml_type==Q4?q4_row_bytes(FFN):q6_row_bytes(FFN);
                addop(ops,p+"ffn_down",z.dw->ggml_type==Q4?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv",{AB(z.DW),&b_s,&b_dummy,&b_d},push_bytes(PCGemm{FFN,H,1,drb,0,BASE(z.DW),0}),(H+63u)/64u);
                addop(ops,p+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(PCN{H}),(H+255u)/256u);
                std::swap(cur,nxt);
            }
            addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB("output_norm.weight"),&b_norm},push_bytes(PCRms{H,1,eps,FBASE("output_norm.weight")}),1);
            append_lm(ops,0);
            return ops;
        };

        auto ppops=build_prefill(seq);auto dtemplate=build_decode(seq);
        if(LM_DISPATCHES!=19u||ppops.size()!=EXPECT_PREFILL||dtemplate.size()!=EXPECT_DECODE)
            throw std::runtime_error("Q2 production graph dispatch census mismatch");
        PreparedChain ppchain=vk.prepare_chain(ppops),dchain=vk.prepare_chain(dtemplate);

        std::vector<Buffer*> transient={
            &b_h0,&b_h1,&b_n1,&b_q,&b_k,&b_v,&b_qr,&b_kr,&b_attn,&b_o,&b_r1,&b_n2,
            &b_g,&b_u,&b_s,&b_d,&b_norm,&b_logits
        };
        auto reset_execution=[&](){
            std::memset(b_kcache.mapped,0,size_t(b_kcache.size));std::memset(b_vcache.mapped,0,size_t(b_vcache.size));
            for(Buffer*p:transient)std::memset(p->mapped,0,size_t(p->size));
            std::memset(b_ids.mapped,0,size_t(b_ids.size));
            std::memcpy(b_ids.mapped,input_ids.data(),input_ids.size()*sizeof(uint32_t));
            uint32_t z=0;std::memcpy(b_dec_id.mapped,&z,sizeof(z));
        };

        struct T1Obs{
            uint32_t layer=0,decode_index=0,position=0;
            std::string op;
            double max_abs=0.0,rmse=0.0;
            uint64_t baseline_ticks=0,candidate_ticks=0;
            double component_speedup=0.0;
            bool finite=false,pass=false;
            std::string baseline_hash,candidate_hash;
        };
        std::vector<T1Obs> obs;
        std::vector<uint32_t> generated;
        const std::array<uint32_t,3> target_layers={0u,13u,27u};
        const std::array<uint32_t,3> target_di={0u,15u,30u};

        auto is_target_di=[&](uint32_t di){
            return di==target_di[0]||di==target_di[1]||di==target_di[2];
        };
        auto execute_slice=[&](const std::vector<DispatchOp>&all,size_t a,size_t b){
            if(a>=b)return;
            std::vector<DispatchOp> part(all.begin()+a,all.begin()+b);
            PreparedChain pc=vk.prepare_chain(part);
            ChainStats st=vk.execute_prepared(pc,part,true);
            if(st.dispatch_count!=part.size()||st.submit_count!=1u){
                vk.destroy_prepared(pc);
                throw std::runtime_error("I002 T1 segmented dispatch census mismatch");
            }
            vk.destroy_prepared(pc);
        };
        auto copy_ffn=[&](Buffer&buf){
            const float*p=reinterpret_cast<const float*>(buf.mapped);
            return std::vector<float>(p,p+FFN);
        };
        auto hash_vec=[&](const std::vector<float>&v){
            return q2_hex64(q2_fnv1a64(v.data(),v.size()*sizeof(float)));
        };
        auto compare_and_record=[&](uint32_t layer,uint32_t di,uint32_t pos,const std::string&op,
                                    const std::vector<float>&baseline,const std::vector<float>&candidate,
                                    uint64_t baseline_ticks,uint64_t candidate_ticks){
            Metrics m=compare_vec(baseline,candidate,0.02,0.005);
            bool finite=p8g_finite(baseline)&&p8g_finite(candidate);
            T1Obs x;x.layer=layer;x.decode_index=di;x.position=pos;x.op=op;
            x.max_abs=m.max_abs;x.rmse=m.rmse;x.baseline_ticks=baseline_ticks;x.candidate_ticks=candidate_ticks;
            x.component_speedup=candidate_ticks?double(baseline_ticks)/double(candidate_ticks):0.0;
            x.finite=finite;x.pass=finite&&m.pass&&baseline_ticks>0&&candidate_ticks>0;
            x.baseline_hash=hash_vec(baseline);x.candidate_hash=hash_vec(candidate);
            obs.push_back(std::move(x));
        };

        reset_execution();
        ChainStats ppst=vk.execute_prepared(ppchain,ppops,false);
        if(ppst.dispatch_count!=EXPECT_PREFILL||ppst.submit_count!=1u)
            throw std::runtime_error("I002 T1 prefill census mismatch");
        const float*lp=reinterpret_cast<const float*>(b_logits.mapped);
        Q2Top2 top=q2_top2(lp,VOC);
        if(!top.finite)throw std::runtime_error("I002 T1 nonfinite prefill logits");
        uint32_t next=top.top1;generated.push_back(next);
        std::memcpy(b_dec_id.mapped,&next,sizeof(next));

        for(uint32_t di=0;di<31u;++di){
            const uint32_t pos=seq+di;
            auto dops=build_decode(pos);
            if(dops.size()!=EXPECT_DECODE)throw std::runtime_error("I002 T1 decode census mismatch");
            if(!is_target_di(di)){
                ChainStats ds=vk.execute_prepared(dchain,dops,true);
                if(ds.dispatch_count!=EXPECT_DECODE||ds.submit_count!=1u)
                    throw std::runtime_error("I002 T1 baseline decode execution mismatch");
            }else{
                size_t cursor=0;
                for(uint32_t layer:target_layers){
                    std::string pfx="L"+(layer<10?std::string("0"):std::string())+std::to_string(layer)+".";
                    const std::string gname=pfx+"ffn_gate",uname=pfx+"ffn_up";
                    size_t gi=dops.size(),ui=dops.size();
                    for(size_t oi=cursor;oi<dops.size();++oi){
                        if(dops[oi].name==gname)gi=oi;
                        if(dops[oi].name==uname){ui=oi;break;}
                    }
                    if(gi>=dops.size()||ui!=gi+1u)throw std::runtime_error("I002 T1 target gate/up not adjacent");
                    execute_slice(dops,cursor,gi);

                    std::vector<DispatchOp> base_pair={dops[gi],dops[ui]};
                    std::vector<DispatchOp> cand_pair=base_pair;
                    for(auto&op:cand_pair){
                        op.spv_path=join_path_p8c(shader_dir,"sa1_q4k_subgroup_splitk.spv");
                        op.gx=(FFN+3u)/4u;op.gy=1u;op.gz=1u;
                    }

                    std::vector<float> bg,bu,cg,cu;
                    ProfileStats bprof{},cprof{};
                    const bool candidate_first=((obs.size()/2u + (session=="B"?1u:0u))%2u)==0u;
                    if(candidate_first){
                        PreparedChain cp=vk.prepare_chain(cand_pair);
                        cprof=vk.execute_profiled(cp,cand_pair,true);
                        if(cprof.chain.dispatch_count!=2u||cprof.chain.submit_count!=1u){vk.destroy_prepared(cp);throw std::runtime_error("I002 T1 candidate pair census");}
                        cg=copy_ffn(b_g);cu=copy_ffn(b_u);vk.destroy_prepared(cp);

                        PreparedChain bp=vk.prepare_chain(base_pair);
                        bprof=vk.execute_profiled(bp,base_pair,true);
                        if(bprof.chain.dispatch_count!=2u||bprof.chain.submit_count!=1u){vk.destroy_prepared(bp);throw std::runtime_error("I002 T1 baseline pair census");}
                        bg=copy_ffn(b_g);bu=copy_ffn(b_u);vk.destroy_prepared(bp);
                    }else{
                        PreparedChain bp=vk.prepare_chain(base_pair);
                        bprof=vk.execute_profiled(bp,base_pair,true);
                        if(bprof.chain.dispatch_count!=2u||bprof.chain.submit_count!=1u){vk.destroy_prepared(bp);throw std::runtime_error("I002 T1 baseline pair census");}
                        bg=copy_ffn(b_g);bu=copy_ffn(b_u);vk.destroy_prepared(bp);

                        PreparedChain cp=vk.prepare_chain(cand_pair);
                        cprof=vk.execute_profiled(cp,cand_pair,true);
                        if(cprof.chain.dispatch_count!=2u||cprof.chain.submit_count!=1u){vk.destroy_prepared(cp);throw std::runtime_error("I002 T1 candidate pair census");}
                        cg=copy_ffn(b_g);cu=copy_ffn(b_u);vk.destroy_prepared(cp);

                        PreparedChain rp=vk.prepare_chain(base_pair);
                        ChainStats rst=vk.execute_prepared(rp,base_pair,true);
                        if(rst.dispatch_count!=2u||rst.submit_count!=1u){vk.destroy_prepared(rp);throw std::runtime_error("I002 T1 restore pair census");}
                        vk.destroy_prepared(rp);
                    }
                    if(bprof.op_ticks.size()!=2u||cprof.op_ticks.size()!=2u)throw std::runtime_error("I002 T1 timestamp census");
                    compare_and_record(layer,di,pos,"gate",bg,cg,bprof.op_ticks[0],cprof.op_ticks[0]);
                    compare_and_record(layer,di,pos,"up",bu,cu,bprof.op_ticks[1],cprof.op_ticks[1]);
                    cursor=ui+1u;
                }
                execute_slice(dops,cursor,dops.size());
            }
            lp=reinterpret_cast<const float*>(b_logits.mapped);
            top=q2_top2(lp,VOC);
            if(!top.finite)throw std::runtime_error("I002 T1 nonfinite decode logits");
            next=top.top1;generated.push_back(next);
            std::memcpy(b_dec_id.mapped,&next,sizeof(next));
        }

        if(obs.size()!=18u)throw std::runtime_error("I002 T1 observation census mismatch");
        bool all_pass=true;for(const auto&o:obs)all_pass=all_pass&&o.pass;
        if(generated.size()!=32u)throw std::runtime_error("I002 T1 generation census mismatch");

        std::ofstream o(out,std::ios::binary);if(!o)throw std::runtime_error("cannot write I002 T1 JSON");
        o<<std::setprecision(15);
        o<<"{\n  \"schema\":\"arcllm.v1.i002.t1.real_model_transfer.v0.1\",\n";
        o<<"  \"session\":\""<<session<<"\",\"workload\":\""<<workload<<"\",\"implementation_commit\":\""<<p8g_escape(implementation_commit)<<"\",\n";
        o<<"  \"candidate_shader\":\"sa1_q4k_subgroup_splitk.spv\",\"candidate_source_blob\":\"56999d88dc1bef6486e7e1908982f6de4b0f9f6a\",\n";
        o<<"  \"frozen_samples\":{\"layers\":[0,13,27],\"decode_indices\":[0,15,30],\"operators\":[\"gate\",\"up\"],\"comparisons_expected\":18},\n";
        o<<"  \"correctness_gate\":{\"max_abs\":0.02,\"rmse\":0.005},\n";
        o<<"  \"generated_token_ids\":[";
        for(size_t i=0;i<generated.size();++i){if(i)o<<",";o<<generated[i];}
        o<<"],\"generated_hash_fnv1a64\":\""<<q2_hex64(q2_fnv1a64(generated.data(),generated.size()*sizeof(uint32_t)))<<"\",\n";
        o<<"  \"comparisons\":[";
        for(size_t i=0;i<obs.size();++i){
            if(i)o<<",";
            const auto&x=obs[i];
            o<<"{\"layer\":"<<x.layer<<",\"decode_index\":"<<x.decode_index<<",\"position\":"<<x.position
             <<",\"operator\":\""<<x.op<<"\",\"max_abs\":"<<x.max_abs<<",\"rmse\":"<<x.rmse
             <<",\"baseline_ticks\":"<<x.baseline_ticks<<",\"candidate_ticks\":"<<x.candidate_ticks
             <<",\"component_speedup\":"<<x.component_speedup
             <<",\"finite\":"<<json_bool(x.finite)<<",\"pass\":"<<json_bool(x.pass)
             <<",\"baseline_hash\":\""<<x.baseline_hash<<"\",\"candidate_hash\":\""<<x.candidate_hash<<"\"}";
        }
        o<<"],\n  \"all_pass\":"<<json_bool(all_pass)<<",\"fresh_execution_requires_authorization\":true\n}\n";
        o.close();
        vk.destroy_prepared(dchain);vk.destroy_prepared(ppchain);
        std::vector<Buffer*>scratch={&b_vcache,&b_kcache,&b_logits,&b_norm,&b_d,&b_s,&b_u,&b_g,&b_n2,&b_r1,&b_o,&b_attn,&b_kr,&b_qr,&b_v,&b_k,&b_q,&b_n1,&b_h1,&b_h0,&b_dummy,&b_dec_id,&b_ids};
        for(Buffer*p:scratch)vk.destroy_buffer(*p);
        for(auto&b:arenas)vk.destroy_buffer(b);
        return 0;
    }catch(const std::exception&e){
        std::ofstream o(out,std::ios::binary);
        if(o)o<<"{\n  \"schema\":\"arcllm.v1.i002.t1.real_model_transfer.v0.1\",\n  \"status\":\"ERROR\",\n  \"error\":\""<<p8g_escape(e.what())<<"\"\n}\n";
        std::cerr<<"I002 T1 error: "<<e.what()<<"\n";return 2;
    }
}
