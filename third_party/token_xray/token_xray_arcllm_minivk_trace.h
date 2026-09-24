#pragma once

// Token X-Ray bridge for ArcLLM's intentionally minimal Vulkan ABI.
// Include this only after ArcLLM's Vk* aliases/structs/constants are declared.

#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct VkQueryPool_T; using VkQueryPool = VkQueryPool_T*;
using VkQueryPoolCreateFlags = VkFlags;
using VkQueryType = int32_t;
using VkQueryPipelineStatisticFlags = VkFlags;
using VkQueryResultFlags = VkFlags;

struct VkQueryPoolCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkQueryPoolCreateFlags flags;
    VkQueryType queryType;
    uint32_t queryCount;
    VkQueryPipelineStatisticFlags pipelineStatistics;
};

static constexpr VkStructureType TXR_VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO = 11;
static constexpr VkQueryType TXR_VK_QUERY_TYPE_TIMESTAMP = 2;
static constexpr VkPipelineStageFlags TXR_VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001;
static constexpr VkPipelineStageFlags TXR_VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000;
static constexpr VkQueryResultFlags TXR_VK_QUERY_RESULT_64_BIT = 0x00000001;
static constexpr VkQueryResultFlags TXR_VK_QUERY_RESULT_WAIT_BIT = 0x00000002;

using TXR_PFN_vkCreateQueryPool = VkResult (WINAPI*)(
    VkDevice, const VkQueryPoolCreateInfo*, const void*, VkQueryPool*);
using TXR_PFN_vkDestroyQueryPool = void (WINAPI*)(
    VkDevice, VkQueryPool, const void*);
using TXR_PFN_vkCmdResetQueryPool = void (WINAPI*)(
    VkCommandBuffer, VkQueryPool, uint32_t, uint32_t);
using TXR_PFN_vkCmdWriteTimestamp = void (WINAPI*)(
    VkCommandBuffer, VkPipelineStageFlags, VkQueryPool, uint32_t);
using TXR_PFN_vkGetQueryPoolResults = VkResult (WINAPI*)(
    VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void*, VkDeviceSize, VkQueryResultFlags);

namespace token_xray {

struct MiniVkFns {
    TXR_PFN_vkCreateQueryPool create_query_pool = nullptr;
    TXR_PFN_vkDestroyQueryPool destroy_query_pool = nullptr;
    TXR_PFN_vkCmdResetQueryPool cmd_reset_query_pool = nullptr;
    TXR_PFN_vkCmdWriteTimestamp cmd_write_timestamp = nullptr;
    TXR_PFN_vkGetQueryPoolResults get_query_pool_results = nullptr;
};

struct MiniTraceMeta {
    std::string run_id;
    std::string model_sha256;
    std::string hardware_profile_id;
    uint32_t token_index = 0;
    uint32_t context_length = 0;
    int64_t input_token_id = -1;
    int64_t output_token_id = -1;
};

struct MiniDispatch {
    uint32_t dispatch_id = 0;
    std::string semantic_node_id;
    std::string runtime_name;
    std::string shader;
    std::string shader_path;
    uint32_t gx=1,gy=1,gz=1,lx=1,ly=1,lz=1,subgroup=0;
    uint32_t q0=0,q1=0;
    uint64_t t0=0,t1=0,duration_ns=0;
};

struct MiniBarrier {
    uint32_t barrier_id=0;
    std::string kind;
    int64_t after_dispatch_id=-1;
    int64_t before_dispatch_id=-1;
    std::string src_stage,dst_stage,src_access,dst_access;
};

class MiniVkTrace {
public:
    MiniVkTrace(VkDevice device, MiniVkFns fns, uint32_t capacity,
                double timestamp_period_ns, uint32_t timestamp_valid_bits)
        : device_(device), fns_(fns), capacity_(capacity),
          period_(timestamp_period_ns), valid_bits_(timestamp_valid_bits) {
        if(!device_||!fns_.create_query_pool||!fns_.destroy_query_pool||
           !fns_.cmd_reset_query_pool||!fns_.cmd_write_timestamp||
           !fns_.get_query_pool_results) throw std::runtime_error("Token X-Ray query API unavailable");
        if(!capacity_||!(period_>0.0)||!valid_bits_||valid_bits_>64)
            throw std::runtime_error("Token X-Ray invalid timestamp configuration");
        count_=2u+2u*capacity_;
        VkQueryPoolCreateInfo ci{};
        ci.sType=TXR_VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        ci.queryType=TXR_VK_QUERY_TYPE_TIMESTAMP;
        ci.queryCount=count_;
        if(fns_.create_query_pool(device_,&ci,nullptr,&pool_)!=VK_SUCCESS||!pool_)
            throw std::runtime_error("Token X-Ray vkCreateQueryPool failed");
    }
    ~MiniVkTrace(){if(pool_)fns_.destroy_query_pool(device_,pool_,nullptr);}
    MiniVkTrace(const MiniVkTrace&)=delete;
    MiniVkTrace& operator=(const MiniVkTrace&)=delete;

    void begin(VkCommandBuffer cb,MiniTraceMeta meta){
        if(active_)throw std::runtime_error("Token X-Ray trace already active");
        meta_=std::move(meta);ds_.clear();bs_.clear();done_=false;
        fns_.cmd_reset_query_pool(cb,pool_,0,count_);
        fns_.cmd_write_timestamp(cb,TXR_VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,pool_,0);
        active_=true;
    }

    uint32_t dispatch_begin(VkCommandBuffer cb,const std::string&semantic,
                            const std::string&runtime_name,const std::string&shader,
                            const std::string&shader_path,uint32_t gx,uint32_t gy,uint32_t gz,
                            uint32_t lx,uint32_t ly,uint32_t lz,uint32_t subgroup){
        if(!active_||ds_.size()>=capacity_||semantic.empty())throw std::runtime_error("Token X-Ray dispatch_begin invalid");
        MiniDispatch d{};d.dispatch_id=uint32_t(ds_.size());d.semantic_node_id=semantic;
        d.runtime_name=runtime_name;d.shader=shader;d.shader_path=shader_path;
        d.gx=gx;d.gy=gy;d.gz=gz;d.lx=lx;d.ly=ly;d.lz=lz;d.subgroup=subgroup;
        d.q0=2u+2u*d.dispatch_id;d.q1=d.q0+1u;
        fns_.cmd_write_timestamp(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,pool_,d.q0);
        ds_.push_back(std::move(d));return ds_.back().dispatch_id;
    }

    void dispatch_end(VkCommandBuffer cb,uint32_t id){
        if(!active_||id>=ds_.size()||id+1u!=ds_.size())throw std::runtime_error("Token X-Ray dispatch_end invalid");
        fns_.cmd_write_timestamp(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,pool_,ds_[id].q1);
    }

    void barrier(const std::string&kind,int64_t after,int64_t before,
                 const std::string&src_stage,const std::string&dst_stage,
                 const std::string&src_access,const std::string&dst_access){
        MiniBarrier b{};b.barrier_id=uint32_t(bs_.size());b.kind=kind;
        b.after_dispatch_id=after;b.before_dispatch_id=before;b.src_stage=src_stage;b.dst_stage=dst_stage;
        b.src_access=src_access;b.dst_access=dst_access;bs_.push_back(std::move(b));
    }

    void end(VkCommandBuffer cb){
        if(!active_)throw std::runtime_error("Token X-Ray trace not active");
        fns_.cmd_write_timestamp(cb,TXR_VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,pool_,1);active_=false;
    }

    void collect_after_fence(){
        if(active_)throw std::runtime_error("Token X-Ray end before collect");
        const uint32_t used=2u+2u*uint32_t(ds_.size());std::vector<uint64_t>ticks(used);
        VkResult vr=fns_.get_query_pool_results(device_,pool_,0,used,ticks.size()*sizeof(uint64_t),
                                                ticks.data(),sizeof(uint64_t),
                                                TXR_VK_QUERY_RESULT_64_BIT|TXR_VK_QUERY_RESULT_WAIT_BIT);
        if(vr!=VK_SUCCESS)throw std::runtime_error("Token X-Ray vkGetQueryPoolResults failed");
        span_ns_=to_ns(delta(ticks[0],ticks[1]));
        for(auto&d:ds_){d.t0=ticks[d.q0];d.t1=ticks[d.q1];d.duration_ns=to_ns(delta(d.t0,d.t1));}
        done_=true;
    }

    void write(const std::string&path)const{
        if(!done_)throw std::runtime_error("Token X-Ray collect before write");
        std::ofstream o(path,std::ios::binary);if(!o)throw std::runtime_error("Token X-Ray trace output open failed");
        uint64_t sum=0;for(const auto&d:ds_)sum+=d.duration_ns;
        uint64_t gap=span_ns_>sum?span_ns_-sum:0;
        o<<"{\n  \"schema_version\":\"0.1\",\"artifact_type\":\"TOKEN_TRACE\",\n";
        o<<"  \"run_id\":\""<<esc(meta_.run_id)<<"\",\n";
        o<<"  \"source_model\":{\"sha256\":\""<<esc(meta_.model_sha256)<<"\"},\n";
        o<<"  \"token\":{\"mode\":\"decode\",\"token_index\":"<<meta_.token_index
         <<",\"context_length\":"<<meta_.context_length<<",\"input_token_id\":"<<nint(meta_.input_token_id)
         <<",\"output_token_id\":"<<nint(meta_.output_token_id)<<"},\n";
        o<<"  \"runtime\":{\"name\":\"ArcLLM-v1-I002\",\"version\":null,\"backend\":\"vulkan\"},\n";
        o<<"  \"device\":{\"profile_id\":\""<<esc(meta_.hardware_profile_id)
         <<"\",\"live_identity\":{\"timestamp_period_ns\":"<<period_<<",\"timestamp_valid_bits\":"<<valid_bits_<<"}},\n";
        o<<"  \"timing\":{\"source\":\"VULKAN_TIMESTAMP_QUERY\",\"timestamp_period_ns\":"<<period_
         <<",\"timestamp_valid_bits\":"<<valid_bits_<<",\"device_span_ns\":"<<span_ns_
         <<",\"sum_dispatch_duration_ns\":"<<sum<<",\"unattributed_device_time_ns\":"<<gap<<"},\n";
        o<<"  \"submissions\":[{\"submit_id\":0,\"queue\":0,\"command_buffer\":\"primary\",\"dispatch_ids\":[";
        for(size_t i=0;i<ds_.size();++i){if(i)o<<",";o<<ds_[i].dispatch_id;}o<<"]}],\n";
        o<<"  \"barriers\":[";for(size_t i=0;i<bs_.size();++i){if(i)o<<",";const auto&b=bs_[i];
        o<<"{\"barrier_id\":"<<b.barrier_id<<",\"kind\":\""<<esc(b.kind)<<"\",\"after_dispatch_id\":"<<nint(b.after_dispatch_id)
         <<",\"before_dispatch_id\":"<<nint(b.before_dispatch_id)<<",\"src_stage\":\""<<esc(b.src_stage)
         <<"\",\"dst_stage\":\""<<esc(b.dst_stage)<<"\",\"src_access\":\""<<esc(b.src_access)
         <<"\",\"dst_access\":\""<<esc(b.dst_access)<<"\"}";}o<<"],\n";
        o<<"  \"dispatches\":[\n";
        for(size_t i=0;i<ds_.size();++i){const auto&d=ds_[i];const std::string bef=bkind_before(d.dispatch_id),aft=bkind_after(d.dispatch_id);
        o<<"    {\"dispatch_id\":"<<d.dispatch_id<<",\"semantic_node_ids\":[\""<<esc(d.semantic_node_id)<<"\"],"
         <<"\"runtime_name\":\""<<esc(d.runtime_name)<<"\",\"kernel\":\""<<esc(d.shader)<<"\",\"shader_path\":\""<<esc(d.shader_path)<<"\","
         <<"\"backend\":\"vulkan\",\"queue\":0,\"command_buffer\":\"primary\","
         <<"\"workgroups\":["<<d.gx<<","<<d.gy<<","<<d.gz<<"],\"local_size\":["<<d.lx<<","<<d.ly<<","<<d.lz<<"],"
         <<"\"subgroup_size\":"<<(d.subgroup?std::to_string(d.subgroup):"null")<<","
         <<"\"query_indices\":{\"start\":"<<d.q0<<",\"end\":"<<d.q1<<"},"
         <<"\"timestamp\":{\"source\":\"VULKAN_TIMESTAMP_QUERY\",\"start_ns\":null,\"end_ns\":null,\"duration_ns\":"<<d.duration_ns
         <<",\"start_tick\":"<<d.t0<<",\"end_tick\":"<<d.t1<<",\"timestamp_period_ns\":"<<period_<<",\"timestamp_valid_bits\":"<<valid_bits_<<"},"
         <<"\"barrier_before\":"<<(bef.empty()?"false":"true")<<",\"barrier_after\":"<<(aft.empty()?"false":"true")<<","
         <<"\"barrier_before_kind\":"<<nstr(bef)<<",\"barrier_after_kind\":"<<nstr(aft)<<",\"counters\":{}}";
        o<<(i+1==ds_.size()?"\n":",\n");}
        o<<"  ]\n}\n";
    }

private:
    uint64_t delta(uint64_t a,uint64_t b)const{
        if(valid_bits_==64)return b-a;uint64_t m=(uint64_t(1)<<valid_bits_)-1u;return(b-a)&m;
    }
    uint64_t to_ns(uint64_t t)const{
        long double x=static_cast<long double>(t)*static_cast<long double>(period_);if(x>=static_cast<long double>(std::numeric_limits<uint64_t>::max()))return std::numeric_limits<uint64_t>::max();
        return uint64_t(std::llround(x));
    }
    static std::string esc(const std::string&s){std::ostringstream o;for(char c:s){if(c=='\\')o<<"\\\\";else if(c=='\"')o<<"\\\"";else if(c=='\n')o<<"\\n";else if(c=='\r')o<<"\\r";else if(c=='\t')o<<"\\t";else o<<c;}return o.str();}
    static std::string nint(int64_t v){return v<0?"null":std::to_string(v);}
    static std::string nstr(const std::string&s){return s.empty()?"null":"\""+esc(s)+"\"";}
    std::string bkind_before(uint32_t id)const{for(const auto&b:bs_)if(b.before_dispatch_id==int64_t(id))return b.kind;return{};}
    std::string bkind_after(uint32_t id)const{for(const auto&b:bs_)if(b.after_dispatch_id==int64_t(id))return b.kind;return{};}

    VkDevice device_=nullptr;MiniVkFns fns_{};uint32_t capacity_=0;double period_=0;uint32_t valid_bits_=0;
    VkQueryPool pool_=nullptr;uint32_t count_=0;bool active_=false,done_=false;MiniTraceMeta meta_{};
    std::vector<MiniDispatch>ds_;std::vector<MiniBarrier>bs_;uint64_t span_ns_=0;
};

} // namespace token_xray
