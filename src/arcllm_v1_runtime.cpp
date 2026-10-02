#include "arcllm_v1_q4_vulkan_backend_v4_runtime.h"

#include <map>
#include <set>
#include <array>
#include <limits>
#include <thread>
#include "../include/arcllm/v1/runtime.h"

struct RuntimeArena { uint64_t start=0,end=0; };
struct RuntimeSlice {
    uint32_t arena=0;
    uint64_t arena_byte_base=0;
    uint64_t source_start=0;
    uint64_t bytes=0;
    uint64_t row_start=0;
    uint64_t row_count=0;
};
struct RuntimeBinding {
    std::string name;
    uint32_t ggml_type=0;
    uint64_t row_bytes=0;
    uint64_t rows=0;
    uint64_t tensor_bytes=0;
    std::vector<RuntimeSlice> slices;
};
static constexpr uint64_t RUNTIME_ARENA_CAP=268435456ull;

static uint64_t runtime_rows_of(const GgufTensorInfo& t){
    if(t.dims.empty())throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t r=1;for(size_t i=1;i<t.dims.size();++i)r*=t.dims[i];return r;
}
static uint64_t runtime_row_bytes(const GgufTensorInfo& t){
    if(t.dims.empty())throw std::runtime_error("tensor has no dims: "+t.name);
    uint64_t n=t.dims[0];
    if(t.ggml_type==0u)return n*4ull;
    if(t.ggml_type==12u){if(n%256ull)throw std::runtime_error("bad Q4_K row: "+t.name);return (n/256ull)*144ull;}
    if(t.ggml_type==14u){if(n%256ull)throw std::runtime_error("bad Q6_K row: "+t.name);return (n/256ull)*210ull;}
    throw std::runtime_error("unsupported ArcLLM runtime tensor type: "+t.name);
}
static std::vector<RuntimeArena> runtime_expected_arenas(){
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
static std::vector<std::pair<uint64_t,uint64_t>> runtime_tensor_pieces(const GgufTensorInfo& t){
    uint64_t rb=runtime_row_bytes(t),rows=runtime_rows_of(t),bytes=rb*rows;
    std::vector<std::pair<uint64_t,uint64_t>> out;
    if(bytes<=RUNTIME_ARENA_CAP){out.push_back({t.offset,t.offset+bytes});return out;}
    uint64_t mr=RUNTIME_ARENA_CAP/rb;if(!mr)throw std::runtime_error("row exceeds arena cap");
    for(uint64_t row=0;row<rows;){
        uint64_t cnt=(std::min)(mr,rows-row);
        uint64_t s=t.offset+row*rb,e=s+cnt*rb;
        out.push_back({s,e});row+=cnt;
    }
    return out;
}
static std::vector<RuntimeArena> runtime_recompute_arenas(const GgufInfo& g){
    struct Piece{uint64_t s,e;};
    std::vector<const GgufTensorInfo*> ts;for(const auto& t:g.tensors)ts.push_back(&t);
    std::sort(ts.begin(),ts.end(),[](auto*a,auto*b){return a->offset<b->offset;});
    std::vector<Piece> pieces;uint64_t payload=0;
    for(auto*t:ts){
        for(auto p:runtime_tensor_pieces(*t))pieces.push_back({p.first,p.second});
        payload=(std::max)(payload,t->offset+tensor_nbytes(*t));
    }
    std::sort(pieces.begin(),pieces.end(),[](const Piece&a,const Piece&b){return a.s<b.s;});
    std::vector<RuntimeArena> out;uint64_t start=0;
    for(const auto&p:pieces){
        if(p.e-start>RUNTIME_ARENA_CAP){
            if(p.s<=start)throw std::runtime_error("cannot pack ArcLLM runtime tensor piece");
            out.push_back({start,p.s});start=p.s;
        }
        if(p.e-start>RUNTIME_ARENA_CAP)throw std::runtime_error("tensor piece exceeds ArcLLM runtime arena");
    }
    if(payload>start)out.push_back({start,payload});
    return out;
}
static bool runtime_arena_equal(const std::vector<RuntimeArena>&a,const std::vector<RuntimeArena>&b){
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(a[i].start!=b[i].start||a[i].end!=b[i].end)return false;
    return true;
}
static std::vector<std::string> runtime_graph_names(){
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
static std::map<std::string,RuntimeBinding> runtime_build_bindings(const GgufInfo&g,const std::vector<RuntimeArena>&arenas,
                                                            uint32_t&pieces,uint32_t&multi,bool&span,bool&coverage){
    pieces=multi=0;span=coverage=true;
    auto names=runtime_graph_names();std::set<std::string> uniq(names.begin(),names.end());
    if(names.size()!=339u||uniq.size()!=339u)throw std::runtime_error("ArcLLM runtime graph census definition mismatch");
    std::map<std::string,RuntimeBinding> out;
    struct S{uint64_t a,b;};std::vector<S> all;
    for(const auto&name:names){
        const auto*t=find_tensor(g,name);if(!t)throw std::runtime_error("missing graph tensor: "+name);
        RuntimeBinding d;d.name=name;d.ggml_type=t->ggml_type;d.row_bytes=runtime_row_bytes(*t);d.rows=runtime_rows_of(*t);d.tensor_bytes=tensor_nbytes(*t);
        uint64_t row=0,cursor=t->offset;
        for(auto p:runtime_tensor_pieces(*t)){
            uint32_t hits=0,ai=0;uint64_t base=0;
            for(uint32_t j=0;j<uint32_t(arenas.size());++j)if(p.first>=arenas[j].start&&p.second<=arenas[j].end){++hits;ai=j;base=p.first-arenas[j].start;}
            if(hits!=1u)throw std::runtime_error("ambiguous ArcLLM runtime binding: "+name);
            uint64_t bytes=p.second-p.first;if(bytes%d.row_bytes)throw std::runtime_error("non-row-aligned ArcLLM runtime tensor piece");
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
struct RuntimeTop2{
    uint32_t top1=0,top2=0;
    float logit1=-std::numeric_limits<float>::infinity();
    float logit2=-std::numeric_limits<float>::infinity();
    bool finite=true;
};
static RuntimeTop2 runtime_top2(const float*p,uint32_t n){
    RuntimeTop2 r;
    for(uint32_t i=0;i<n;++i){
        const float v=p[i];
        if(!std::isfinite(v)){r.finite=false;continue;}
        if(v>r.logit1){r.logit2=r.logit1;r.top2=r.top1;r.logit1=v;r.top1=i;}
        else if(v>r.logit2){r.logit2=v;r.top2=i;}
    }
    if(!std::isfinite(r.logit1)||!std::isfinite(r.logit2))r.finite=false;
    return r;
}
arcllm::v1::runtime::RunResult arcllm::v1::runtime::generate(const RunRequest& request){
    const std::string& model=request.model_path;
    const std::string& shader_dir=request.shader_dir;
    const std::string& sidecar=request.sidecar_path;
    if(model.empty()||shader_dir.empty())
        throw std::runtime_error("ArcLLM runtime requires model_path and shader_dir");
    if(request.input_token_ids.empty())
        throw std::runtime_error("ArcLLM runtime requires at least one input token");
    if(request.max_new_tokens==0u)
        throw std::runtime_error("ArcLLM runtime requires max_new_tokens >= 1");
        GgufInfo gguf=GgufReader(model).read();
        TensorStore store;TensorStoreReport ts=store.inspect(model,gguf);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("ArcLLM runtime TensorStore invariants failed");

        const uint32_t H=3584,QH=28,KVH=4,HD=128,KV=512,FFN=18944,MAXSEQ=256,MAXCTX=4096,VOC=152064;
        const uint32_t LAYERS=28,F32=0,Q4=12,Q6=14;
        const uint32_t EMB_BOUNDARY=133152,OUT_BOUNDARY=91304,EMB_RB=2016,OUT_RB=2940,LM_CHUNK=8192;
        const uint32_t LM_DISPATCHES=(VOC+LM_CHUNK-1u)/LM_CHUNK;
        const uint32_t EXPECT_PREFILL=441,EXPECT_DECODE=469;
        const uint64_t RUNTIME_WEIGHT_BYTES=4677120000ull,RUNTIME_KV_BYTES=469762048ull,RUNTIME_WORKING_BYTES=200888324ull;
        const uint64_t RUNTIME_TOTAL_RESIDENT=5347770372ull,RUNTIME_USABLE_BUDGET=16374562816ull;
        const uint32_t R1_CAPTURE_STATES=LAYERS+1u;
        const uint64_t R1_CAPTURE_BYTES=uint64_t(R1_CAPTURE_STATES)*H*sizeof(float);

        if(gguf.tensor_count!=339u||gguf.tensors.size()!=339u)throw std::runtime_error("ArcLLM runtime exact tensor census mismatch");
        auto bc=gguf.scalars.find("qwen2.block_count");
        if(bc==gguf.scalars.end()||std::stoul(bc->second)!=LAYERS)throw std::runtime_error("ArcLLM runtime block count mismatch");

        auto arenas_plan=runtime_recompute_arenas(gguf);
        if(!runtime_arena_equal(arenas_plan,runtime_expected_arenas()))throw std::runtime_error("ArcLLM runtime arena contract mismatch");
        uint32_t piece_count=0,multi_count=0;bool span=false,coverage=false;
        auto bindings=runtime_build_bindings(gguf,arenas_plan,piece_count,multi_count,span,coverage);
        if(bindings.size()!=339u||piece_count!=341u||multi_count!=2u||!span||!coverage)
            throw std::runtime_error("ArcLLM runtime graph binding mismatch");

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
                throw std::runtime_error("ArcLLM runtime mixed-quant layer obstruction: "+std::to_string(l));
            for(const auto&n:std::vector<std::string>{z.AN,z.QW,z.KW,z.VW,z.QB,z.KB,z.VB,z.OW,z.FN,z.GW,z.UW,z.DW})
                if(bindings.at(n).slices.size()!=1u)throw std::runtime_error("ArcLLM runtime decoder tensor unexpectedly segmented: "+n);
            lt[l]=std::move(z);
        }

        const auto*emb=find_tensor(gguf,"token_embd.weight");
        const auto*outw=find_tensor(gguf,"output.weight");
        const auto*outn=find_tensor(gguf,"output_norm.weight");
        require_dims(emb,Q4,H,VOC,"token_embd.weight");require_dims(outw,Q6,H,VOC,"output.weight");require_vec(outn,F32,H,"output_norm.weight");
        const auto&EB=bindings.at("token_embd.weight");const auto&OB=bindings.at("output.weight");
        if(EB.slices.size()!=2u||OB.slices.size()!=2u||bindings.at("output_norm.weight").slices.size()!=1u)
            throw std::runtime_error("ArcLLM runtime endpoint segmentation mismatch");
        if(EB.slices[0].row_count!=EMB_BOUNDARY||EB.slices[1].row_start!=EMB_BOUNDARY||
           OB.slices[0].row_count!=OUT_BOUNDARY||OB.slices[1].row_start!=OUT_BOUNDARY||
           EB.row_bytes!=EMB_RB||OB.row_bytes!=OUT_RB)
            throw std::runtime_error("ArcLLM runtime endpoint segmented geometry mismatch");
        for(const auto&s:EB.slices)if(s.arena_byte_base!=0u)throw std::runtime_error("ArcLLM runtime embedding segment does not start at arena base");
        for(const auto&s:OB.slices)if(s.arena_byte_base!=0u)throw std::runtime_error("ArcLLM runtime output segment does not start at arena base");

        const uint8_t*payload=store.mapped_base()+gguf.data_offset;
        VkRuntime vk;vk.init();
        std::vector<Buffer> arenas;arenas.reserve(19);
        uint64_t weight_requested=0;
        for(const auto&a:arenas_plan){uint64_t bytes=a.end-a.start;weight_requested+=bytes;arenas.push_back(vk.make_buffer(bytes,payload+a.start));}
        if(weight_requested!=RUNTIME_WEIGHT_BYTES)throw std::runtime_error("ArcLLM runtime weight residency bytes mismatch");

        auto BIND=[&](const std::string&n)->const RuntimeBinding&{return bindings.at(n);};
        auto AB=[&](const std::string&n)->Buffer*{const auto&b=BIND(n);if(b.slices.size()!=1u)throw std::runtime_error("ArcLLM runtime buffer lookup on segmented tensor: "+n);return &arenas.at(b.slices[0].arena);};
        auto BASE=[&](const std::string&n)->uint32_t{const auto&b=BIND(n);if(b.slices.size()!=1u)throw std::runtime_error("ArcLLM runtime base lookup on segmented tensor: "+n);uint64_t x=b.slices[0].arena_byte_base;if(x>0xffffffffull)throw std::runtime_error("ArcLLM runtime base exceeds uint32");return uint32_t(x);};
        auto FBASE=[&](const std::string&n)->uint32_t{uint32_t b=BASE(n);if(b%4u)throw std::runtime_error("ArcLLM runtime unaligned F32 binding");return b/4u;};

        uint64_t working_requested=0,kv_requested=0;
        auto wb=[&](uint64_t bytes,const void*init){working_requested+=bytes;return vk.make_buffer(bytes,init);};
        auto wf=[&](uint64_t elems){return wb(elems*sizeof(float),nullptr);};
        auto kf=[&](uint64_t elems){uint64_t bytes=elems*sizeof(float);kv_requested+=bytes;return vk.make_buffer(bytes);};


        std::vector<uint32_t> input_ids=request.input_token_ids;
        if(input_ids.size()>MAXSEQ)
            throw std::runtime_error("ArcLLM runtime prefill token count exceeds current model/runtime limit");
        const uint32_t seq=uint32_t(input_ids.size());
        if(uint64_t(seq)+uint64_t(request.max_new_tokens)-1ull>MAXCTX)
            throw std::runtime_error("ArcLLM runtime request exceeds KV context limit");
        for(uint32_t id:input_ids)if(id>=VOC)
            throw std::runtime_error("ArcLLM runtime input token out of vocabulary");
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
        if(kv_requested!=RUNTIME_KV_BYTES)throw std::runtime_error("ArcLLM runtime KV residency bytes mismatch");

        uint64_t representation_capture_requested=0;
        std::vector<Buffer> r1_capture_buffers;
        if(request.capture_representation_trajectory){
            r1_capture_buffers.reserve(R1_CAPTURE_STATES);
            for(uint32_t i=0;i<R1_CAPTURE_STATES;++i){
                representation_capture_requested+=uint64_t(H)*sizeof(float);
                r1_capture_buffers.push_back(vk.make_buffer(uint64_t(H)*sizeof(float)));
            }
            if(representation_capture_requested!=R1_CAPTURE_BYTES)
                throw std::runtime_error("ArcLLM runtime R1 capture allocation mismatch");
        }

        const uint64_t total_requested=weight_requested+kv_requested+working_requested+representation_capture_requested;
        if(working_requested>RUNTIME_WORKING_BYTES||total_requested>RUNTIME_USABLE_BUDGET)
            throw std::runtime_error("ArcLLM runtime requested residency exceeds validated envelope");
        if(!request.capture_representation_trajectory&&total_requested>RUNTIME_TOTAL_RESIDENT)
            throw std::runtime_error("ArcLLM runtime baseline residency drift");

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
        struct PCR1Capture{uint32_t row,hidden;};

        auto addop=[&](std::vector<DispatchOp>&ops,const std::string&name,const std::string&sh,std::vector<Buffer*>bufs,std::vector<uint8_t>push,uint32_t gx,uint32_t gy=1,uint32_t gz=1){
            ops.push_back({name,join_path_p8c(shader_dir,sh),std::move(bufs),std::move(push),gx,gy,gz});
        };
        Buffer*emb0=&arenas.at(EB.slices[0].arena);Buffer*emb1=&arenas.at(EB.slices[1].arena);
        Buffer*out0=&arenas.at(OB.slices[0].arena);Buffer*out1=&arenas.at(OB.slices[1].arena);

        auto append_r1_capture=[&](std::vector<DispatchOp>&ops,Buffer*src,uint32_t state_index,uint32_t row){
            if(!request.capture_representation_trajectory)return;
            if(state_index>=r1_capture_buffers.size())throw std::runtime_error("R1 capture state index overflow");
            addop(ops,"token_xray_r1_capture."+std::to_string(state_index),
                  "token_xray_r1_capture_row.spv",{src,&r1_capture_buffers.at(state_index)},
                  push_bytes(PCR1Capture{row,H}),(H+255u)/256u);
        };

        auto append_lm=[&](std::vector<DispatchOp>&ops,uint32_t x_base){
            for(uint32_t rs=0;rs<VOC;rs+=LM_CHUNK){
                uint32_t rc=(std::min)(LM_CHUNK,VOC-rs);
                addop(ops,"lm_head","p8q1_lmhead_q6k_segmented_chunk.spv",{out0,out1,&b_norm,&b_logits},
                      push_bytes(PCLMSeg{H,OUT_RB,rs,rc,OUT_BOUNDARY,x_base}),(rc+63u)/64u);
            }
        };

        std::array<const uint8_t*,14> q4_src_cpu{};
        std::array<Buffer*,14> q4_src_gpu{};
        std::array<uint32_t,14> q4_src_base{};
        for(uint32_t slot=0;slot<uint32_t(kQ4Layers.size());++slot){
            const uint32_t l=kQ4Layers[slot];
            const LT& z=lt[l];
            if(z.dw->ggml_type!=Q4)throw std::runtime_error("frozen Q4-down layer map drift");
            q4_src_cpu[slot]=payload+z.dw->offset;
            q4_src_gpu[slot]=AB(z.DW);
            q4_src_base[slot]=BASE(z.DW);
        }

        Q4VulkanBackendV4 q4_backend(
            vk,q4_src_cpu,q4_src_gpu,q4_src_base,shader_dir,sidecar);
        reg::PrimitiveRegistry q4_registry;
        reg::RegistryError q4_reg_error{};
        if(q4_registry.add_bundle(q4reg::q4k_down_reference_bundle(),&q4_reg_error)!=reg::RegistryStatus::OK)
            throw std::runtime_error("FULL_ARCLLM_RUNTIME Q4 registry bind failed");
        bind::BindingState q4_binding_state{};

        auto q4_decide=[&](uint64_t future_reuse)->gp::PolicyDecision{
            gp::PrimitiveRuntimeState ps[]={
                {q4reg::kPrimitiveA,false,true,true,true,true},
                {q4reg::kPrimitiveB,q4_backend.resident(),q4_backend.validated(),true,
                 q4_backend.resident()&&q4_backend.validated(),true}
            };
            gp::AcquisitionRuntimeState as[]={
                {q4reg::kAcquirePrimary,true,false},
                {q4reg::kAcquireSecondary,q4_backend.p3_available(),false},
                {q4reg::kAcquireTertiary,true,false},
                {q4reg::kAcquireDisabledWarm,false,false}
            };
            gp::PolicyRequest req{};
            req.capability=q4reg::kCapability;
            req.profile=(request.evidence_profile==arcllm::v1::runtime::EvidenceProfile::PROFILE_0)?q4reg::kProfile0:q4reg::kProfile1;
            req.request_within_capability_evidence_scope=request.request_within_validated_domain;
            req.model_loaded=true;
            req.future_reuse_known=true;
            req.future_reuse_units=future_reuse;
            req.acquisition_allowed=true;
            req.primitive_states=ps;
            req.primitive_state_count=2;
            req.acquisition_states=as;
            req.acquisition_state_count=4;
            return gp::evaluate(q4_registry,req);
        };

        auto build_prefill=[&](uint32_t seq){
            std::vector<DispatchOp>ops;
            addop(ops,"token_embedding","p8c_embedding_q4k_segmented_probe.spv",{emb0,emb1,&b_ids,&b_h0},
                  push_bytes(PCEmbSeg{H,seq,EMB_RB,EMB_BOUNDARY}),(seq*H+255u)/256u);
            append_r1_capture(ops,&b_h0,0u,seq-1u);
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
                append_r1_capture(ops,nxt,l+1u,seq-1u);
                std::swap(cur,nxt);
            }
            addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB("output_norm.weight"),&b_norm},push_bytes(PCRms{H,seq,eps,FBASE("output_norm.weight")}),seq);
            append_lm(ops,(seq-1u)*H);
            return ops;
        };

        auto build_decode=[&](uint32_t pos,bind::PrimitiveHandle q4_route,bool capture_this_step){
            std::vector<DispatchOp>ops;
            addop(ops,"token_embedding","p8c_embedding_q4k_segmented_probe.spv",{emb0,emb1,&b_dec_id,&b_h0},
                  push_bytes(PCEmbSeg{H,1,EMB_RB,EMB_BOUNDARY}),(H+255u)/256u);
            if(capture_this_step)append_r1_capture(ops,&b_h0,0u,0u);
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
                if(request.request_within_validated_domain){
                    addop(ops,p+"ffn_gate","sa1_q4k_subgroup_splitk.spv",
                          {AB(z.GW),&b_n2,&b_dummy,&b_g},push_bytes(PCGemm{H,FFN,1,q4_row_bytes(H),0,BASE(z.GW),0}),
                          ((FFN+3u)/4u));
                    addop(ops,p+"ffn_up","sa1_q4k_subgroup_splitk.spv",
                          {AB(z.UW),&b_n2,&b_dummy,&b_u},push_bytes(PCGemm{H,FFN,1,q4_row_bytes(H),0,BASE(z.UW),0}),
                          ((FFN+3u)/4u));
                }else{
                    addop(ops,p+"ffn_gate","p7_q4k_gemm_2d.spv",
                          {AB(z.GW),&b_n2,&b_dummy,&b_g},push_bytes(PCGemm{H,FFN,1,q4_row_bytes(H),0,BASE(z.GW),0}),
                          ((FFN+63u)/64u));
                    addop(ops,p+"ffn_up","p7_q4k_gemm_2d.spv",
                          {AB(z.UW),&b_n2,&b_dummy,&b_u},push_bytes(PCGemm{H,FFN,1,q4_row_bytes(H),0,BASE(z.UW),0}),
                          ((FFN+63u)/64u));
                }
                addop(ops,p+"swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(PCN{FFN}),(FFN+255u)/256u);
                if(z.dw->ggml_type==Q4){
                    q4_backend.append_ffn_down_op(
                        ops,p+"ffn_down",q4_route,l,AB(z.DW),BASE(z.DW),b_s,b_dummy,b_d);
                }else{
                    addop(ops,p+"ffn_down","p7_q6k_gemm_2d.spv",
                          {AB(z.DW),&b_s,&b_dummy,&b_d},
                          push_bytes(PCGemm{FFN,H,1,q6_row_bytes(FFN),0,BASE(z.DW),0}),
                          (H+63u)/64u);
                }
                addop(ops,p+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(PCN{H}),(H+255u)/256u);
                if(capture_this_step)append_r1_capture(ops,nxt,l+1u,0u);
                std::swap(cur,nxt);
            }
            addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB("output_norm.weight"),&b_norm},push_bytes(PCRms{H,1,eps,FBASE("output_norm.weight")}),1);
            append_lm(ops,0);
            return ops;
        };


        auto ppops=build_prefill(seq);
        const uint32_t expected_prefill_dispatches=EXPECT_PREFILL+(request.capture_representation_trajectory?R1_CAPTURE_STATES:0u);
        if(LM_DISPATCHES!=19u||ppops.size()!=expected_prefill_dispatches)
            throw std::runtime_error("ArcLLM runtime prefill graph topology mismatch");
        PreparedChain ppchain=vk.prepare_chain(ppops);

        std::vector<Buffer*> transient={
            &b_h0,&b_h1,&b_n1,&b_q,&b_k,&b_v,&b_qr,&b_kr,&b_attn,&b_o,&b_r1,&b_n2,
            &b_g,&b_u,&b_s,&b_d,&b_norm,&b_logits
        };
        std::memset(b_kcache.mapped,0,size_t(b_kcache.size));
        std::memset(b_vcache.mapped,0,size_t(b_vcache.size));
        for(Buffer*p:transient)std::memset(p->mapped,0,size_t(p->size));
        std::memset(b_ids.mapped,0,size_t(b_ids.size));
        std::memcpy(b_ids.mapped,input_ids.data(),input_ids.size()*sizeof(uint32_t));
        uint32_t z0=0;std::memcpy(b_dec_id.mapped,&z0,sizeof(z0));

        ChainStats ps=vk.execute_prepared(ppchain,ppops,false);
        if(ps.dispatch_count!=expected_prefill_dispatches||ps.submit_count!=1u)
            throw std::runtime_error("ArcLLM runtime prefill dispatch topology mismatch");

        const float* lp=reinterpret_cast<const float*>(b_logits.mapped);
        RuntimeTop2 top=runtime_top2(lp,VOC);
        if(!top.finite)throw std::runtime_error("ArcLLM runtime non-finite prefill logits");

        RunResult result;
        result.generated_token_ids.reserve(request.max_new_tokens);
        result.generated_token_ids.push_back(top.top1);
        result.stats.prefill_dispatches=EXPECT_PREFILL;
        result.stats.prefill_submits=ps.submit_count;
        result.stats.representation_prefill_capture_dispatches=
            request.capture_representation_trajectory?R1_CAPTURE_STATES:0u;
        result.stats.finite=true;

        auto collect_r1_states=[&](const char*phase,uint32_t token_id,uint32_t token_position){
            if(!request.capture_representation_trajectory)return;
            for(uint32_t state_index=0;state_index<R1_CAPTURE_STATES;++state_index){
                RepresentationState s;
                s.phase=phase;
                s.state_index=state_index;
                s.layer_index=(state_index==0u)?-1:std::int32_t(state_index-1u);
                s.state_point=(state_index==0u)?"embedding_output":
                    ("block_output:"+std::to_string(state_index-1u));
                s.token_id=token_id;
                s.token_position=token_position;
                s.hidden_dimension=H;
                s.source_dtype="float32";
                const float*src=reinterpret_cast<const float*>(r1_capture_buffers.at(state_index).mapped);
                s.values.assign(src,src+H);
                for(float v:s.values)if(!std::isfinite(v))
                    throw std::runtime_error("R1 captured non-finite representation state");
                result.representation_states.push_back(std::move(s));
            }
        };
        collect_r1_states("prefill",input_ids.back(),seq-1u);

        uint32_t next=top.top1;
        std::memcpy(b_dec_id.mapped,&next,sizeof(next));

        PreparedChain dchain{};
        bool dchain_ready=false;
        uint64_t dchain_route=0;
        for(uint32_t di=0;di+1u<request.max_new_tokens;++di){
            const uint64_t future_reuse=uint64_t(request.max_new_tokens-1u-di);
            const gp::PolicyDecision decision=q4_decide(future_reuse);
            if(decision.status!=gp::PolicyStatus::OK)
                throw std::runtime_error("ArcLLM runtime Q4 policy decision not OK");
            if(decision.lifecycle==gp::LifecycleAction::ACQUIRE)++result.stats.acquire_events;
            if(decision.lifecycle==gp::LifecycleAction::EVICT)++result.stats.evict_events;

            const bind::ApplyResult applied=bind::apply_decision(
                q4_registry,q4reg::kCapability,decision,q4_backend,q4_binding_state);
            if(!applied.ready||applied.backend_status!=bind::BackendStatus::OK)
                throw std::runtime_error("ArcLLM runtime Q4 binding not ready");
            if(applied.primitive.opaque==q4reg::kPrimitiveA.value)++result.stats.route_a_steps;
            else if(applied.primitive.opaque==q4reg::kPrimitiveB.value)++result.stats.route_b_steps;
            else throw std::runtime_error("ArcLLM runtime unknown Q4 primitive");

            const uint32_t pos=seq+di;
            const uint32_t decode_input_token=next;
            const bool capture_this_step=request.capture_representation_trajectory&&di==0u;
            auto dops=build_decode(pos,applied.primitive,capture_this_step);
            const uint32_t expected_decode_dispatches=EXPECT_DECODE+(capture_this_step?R1_CAPTURE_STATES:0u);
            if(dops.size()!=expected_decode_dispatches)
                throw std::runtime_error("ArcLLM runtime decode graph topology mismatch");
            if(!dchain_ready||dchain_route!=applied.primitive.opaque){
                if(dchain_ready)vk.destroy_prepared(dchain);
                dchain=vk.prepare_chain(dops);
                dchain_ready=true;
                dchain_route=applied.primitive.opaque;
            }
            ChainStats ds=vk.execute_prepared(dchain,dops,true);
            if(ds.dispatch_count!=expected_decode_dispatches||ds.submit_count!=1u)
                throw std::runtime_error("ArcLLM runtime decode dispatch topology mismatch");
            if(result.stats.decode_steps==0u){
                result.stats.decode_dispatches_per_step=EXPECT_DECODE;
                result.stats.decode_submits_per_step=ds.submit_count;
            }else if(result.stats.decode_dispatches_per_step!=EXPECT_DECODE||
                     result.stats.decode_submits_per_step!=ds.submit_count){
                throw std::runtime_error("ArcLLM runtime decode topology changed within request");
            }
            if(capture_this_step){
                result.stats.representation_decode_capture_dispatches+=R1_CAPTURE_STATES;
                collect_r1_states("decode",decode_input_token,pos);
            }
            ++result.stats.decode_steps;

            lp=reinterpret_cast<const float*>(b_logits.mapped);
            top=runtime_top2(lp,VOC);
            if(!top.finite)throw std::runtime_error("ArcLLM runtime non-finite decode logits");
            next=top.top1;
            result.generated_token_ids.push_back(next);
            std::memcpy(b_dec_id.mapped,&next,sizeof(next));
        }
        if(dchain_ready)vk.destroy_prepared(dchain);

        const gp::PolicyDecision close_decision=q4_decide(0u);
        if(close_decision.status!=gp::PolicyStatus::OK)
            throw std::runtime_error("ArcLLM runtime close policy not OK");
        if(close_decision.lifecycle==gp::LifecycleAction::EVICT)++result.stats.evict_events;
        const bind::ApplyResult close_applied=bind::apply_decision(
            q4_registry,q4reg::kCapability,close_decision,q4_backend,q4_binding_state);
        if(!close_applied.ready||close_applied.backend_status!=bind::BackendStatus::OK)
            throw std::runtime_error("ArcLLM runtime close binding not ready");
        if(q4_backend.resident())
            throw std::runtime_error("ArcLLM runtime represented Q4 image still resident after request close");

        const auto bcounters=q4_backend.counters();
        result.stats.b_allocations=bcounters.b_allocations;
        result.stats.b_materializations=bcounters.b_materializations;
        result.stats.b_validations=bcounters.b_validations;
        result.stats.b_releases=bcounters.b_releases;
        result.stats.p1_calls=bcounters.p1_calls;
        result.stats.p3_calls=bcounters.p3_calls;
        result.stats.p0_calls=bcounters.p0_calls;

        if(result.generated_token_ids.size()!=request.max_new_tokens)
            throw std::runtime_error("ArcLLM runtime generated-token count mismatch");

        vk.destroy_prepared(ppchain);
        std::vector<Buffer*>scratch={&b_vcache,&b_kcache,&b_logits,&b_norm,&b_d,&b_s,&b_u,&b_g,&b_n2,&b_r1,&b_o,&b_attn,&b_kr,&b_qr,&b_v,&b_k,&b_q,&b_n1,&b_h1,&b_h0,&b_dummy,&b_dec_id,&b_ids};
        for(Buffer*p:scratch)vk.destroy_buffer(*p);
        for(auto&b:r1_capture_buffers)vk.destroy_buffer(b);
        for(auto&b:arenas)vk.destroy_buffer(b);
        return result;
}
