#include "arcllm_v1_m3_counter_shim.h"
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace {
void seterr(char* dst,size_t n,const std::string& s){
    if(!dst||!n)return;
    const size_t m=(std::min)(n-1,s.size());
    std::memcpy(dst,s.data(),m);dst[m]='\0';
}
std::string vrmsg(const char* where,VkResult vr){
    std::ostringstream o;o<<where<<" failed VkResult="<<int(vr);return o.str();
}
std::string hex_uuid(const uint8_t* p,size_t n){
    static const char* h="0123456789abcdef";std::string s;s.resize(n*2);
    for(size_t i=0;i<n;++i){s[2*i]=h[p[i]>>4];s[2*i+1]=h[p[i]&15];}
    return s;
}
}

extern "C" int m3_perf_create_device(
    void* physical_device,uint32_t queue_family,float priority,void** out_device,char* error,size_t error_size){
    if(!physical_device||!out_device){seterr(error,error_size,"m3_perf_create_device invalid args");return 1;}
    VkPhysicalDevice phys=reinterpret_cast<VkPhysicalDevice>(physical_device);
    VkPhysicalDevicePerformanceQueryFeaturesKHR perf{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR};
    VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    f2.pNext=&perf;vkGetPhysicalDeviceFeatures2(phys,&f2);
    if(!perf.performanceCounterQueryPools){seterr(error,error_size,"performanceCounterQueryPools not supported");return 2;}
    perf.performanceCounterQueryPools=VK_TRUE;
    perf.performanceCounterMultipleQueryPools=VK_FALSE;
    VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    q.queueFamilyIndex=queue_family;q.queueCount=1;q.pQueuePriorities=&priority;
    const char* ext=VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME;
    VkDeviceCreateInfo d{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    d.pNext=&perf;d.queueCreateInfoCount=1;d.pQueueCreateInfos=&q;
    d.enabledExtensionCount=1;d.ppEnabledExtensionNames=&ext;
    VkDevice dev=VK_NULL_HANDLE;VkResult vr=vkCreateDevice(phys,&d,nullptr,&dev);
    if(vr!=VK_SUCCESS){seterr(error,error_size,vrmsg("vkCreateDevice(perf)",vr));return 3;}
    *out_device=reinterpret_cast<void*>(dev);return 0;
}

extern "C" int m3_perf_create_query_pool(
    void* instance,void* physical_device,void* device,uint32_t queue_family,
    const uint32_t* counter_indices,uint32_t counter_count,uint32_t query_count,
    void** out_query_pool,uint32_t* out_pass_count,M3PerfCounterMeta* out_meta,char* error,size_t error_size){
    if(!instance||!physical_device||!device||!counter_indices||!counter_count||!query_count||!out_query_pool||!out_pass_count||!out_meta){
        seterr(error,error_size,"m3_perf_create_query_pool invalid args");return 1;
    }
    VkInstance inst=reinterpret_cast<VkInstance>(instance);
    VkPhysicalDevice phys=reinterpret_cast<VkPhysicalDevice>(physical_device);
    VkDevice dev=reinterpret_cast<VkDevice>(device);
    auto enumerate=reinterpret_cast<PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR>(
        vkGetInstanceProcAddr(inst,"vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR"));
    auto getpasses=reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR>(
        vkGetInstanceProcAddr(inst,"vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR"));
    if(!enumerate||!getpasses){seterr(error,error_size,"performance query instance entrypoints unavailable");return 2;}

    uint32_t n=0;VkResult vr=enumerate(phys,queue_family,&n,nullptr,nullptr);
    if(vr!=VK_SUCCESS){seterr(error,error_size,vrmsg("enumerate counter count",vr));return 3;}
    std::vector<VkPerformanceCounterKHR> cs(n);
    std::vector<VkPerformanceCounterDescriptionKHR> ds(n);
    for(uint32_t i=0;i<n;++i){cs[i].sType=VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR;ds[i].sType=VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR;}
    if(n){vr=enumerate(phys,queue_family,&n,cs.data(),ds.data());if(vr!=VK_SUCCESS){seterr(error,error_size,vrmsg("enumerate counters",vr));return 4;}}
    for(uint32_t i=0;i<counter_count;++i){
        const uint32_t ix=counter_indices[i];if(ix>=n){seterr(error,error_size,"selected counter index out of range");return 5;}
        auto& m=out_meta[i];std::memset(&m,0,sizeof(m));m.index=ix;m.storage=uint32_t(cs[ix].storage);m.unit=uint32_t(cs[ix].unit);m.scope=uint32_t(cs[ix].scope);m.flags=uint32_t(ds[ix].flags);
        std::strncpy(m.name,ds[ix].name,sizeof(m.name)-1);std::strncpy(m.category,ds[ix].category,sizeof(m.category)-1);std::strncpy(m.description,ds[ix].description,sizeof(m.description)-1);
        const std::string u=hex_uuid(cs[ix].uuid,VK_UUID_SIZE);std::strncpy(m.uuid_hex,u.c_str(),sizeof(m.uuid_hex)-1);
        if(cs[ix].scope!=VK_PERFORMANCE_COUNTER_SCOPE_COMMAND_KHR){seterr(error,error_size,"selected counter is not COMMAND scope");return 6;}
    }

    VkQueryPoolPerformanceCreateInfoKHR perf{VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR};
    perf.queueFamilyIndex=queue_family;perf.counterIndexCount=counter_count;perf.pCounterIndices=counter_indices;
    uint32_t passes=0;getpasses(phys,&perf,&passes);if(!passes){seterr(error,error_size,"driver returned zero counter passes");return 7;}

    VkQueryPoolCreateInfo q{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    q.pNext=&perf;q.queryType=VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR;q.queryCount=query_count;
    VkQueryPool pool=VK_NULL_HANDLE;vr=vkCreateQueryPool(dev,&q,nullptr,&pool);
    if(vr!=VK_SUCCESS){seterr(error,error_size,vrmsg("vkCreateQueryPool(perf)",vr));return 8;}
    *out_query_pool=reinterpret_cast<void*>(pool);*out_pass_count=passes;return 0;
}

extern "C" int m3_perf_acquire_lock(void* device,uint64_t timeout_ns,char* error,size_t error_size){
    VkDevice dev=reinterpret_cast<VkDevice>(device);
    auto fn=reinterpret_cast<PFN_vkAcquireProfilingLockKHR>(vkGetDeviceProcAddr(dev,"vkAcquireProfilingLockKHR"));
    if(!fn){seterr(error,error_size,"vkAcquireProfilingLockKHR unavailable");return 1;}
    VkAcquireProfilingLockInfoKHR info{VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR};info.timeout=timeout_ns;
    VkResult vr=fn(dev,&info);if(vr!=VK_SUCCESS){seterr(error,error_size,vrmsg("vkAcquireProfilingLockKHR",vr));return 2;}return 0;
}
extern "C" void m3_perf_release_lock(void* device){
    VkDevice dev=reinterpret_cast<VkDevice>(device);
    auto fn=reinterpret_cast<PFN_vkReleaseProfilingLockKHR>(vkGetDeviceProcAddr(dev,"vkReleaseProfilingLockKHR"));
    if(fn)fn(dev);
}
extern "C" void m3_perf_cmd_begin_query(void* cb,void* pool,uint32_t query){
    vkCmdBeginQuery(reinterpret_cast<VkCommandBuffer>(cb),reinterpret_cast<VkQueryPool>(pool),query,0);
}
extern "C" void m3_perf_cmd_end_query(void* cb,void* pool,uint32_t query){
    vkCmdEndQuery(reinterpret_cast<VkCommandBuffer>(cb),reinterpret_cast<VkQueryPool>(pool),query);
}
extern "C" int m3_perf_submit_pass(void* queue,void* cb,void* fence,uint32_t pass_index,char* error,size_t error_size){
    VkPerformanceQuerySubmitInfoKHR pi{VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_SUBMIT_INFO_KHR};pi.counterPassIndex=pass_index;
    VkCommandBuffer command=reinterpret_cast<VkCommandBuffer>(cb);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.pNext=&pi;si.commandBufferCount=1;si.pCommandBuffers=&command;
    VkResult vr=vkQueueSubmit(reinterpret_cast<VkQueue>(queue),1,&si,reinterpret_cast<VkFence>(fence));
    if(vr!=VK_SUCCESS){seterr(error,error_size,vrmsg("vkQueueSubmit(perf pass)",vr));return 1;}return 0;
}
extern "C" int m3_perf_get_results(
    void* device,void* query_pool,uint32_t query_count,uint32_t counter_count,
    const M3PerfCounterMeta* meta,M3PerfCounterValue* out_values,char* error,size_t error_size){
    if(!device||!query_pool||!query_count||!counter_count||!meta||!out_values){seterr(error,error_size,"m3_perf_get_results invalid args");return 1;}
    VkDevice dev=reinterpret_cast<VkDevice>(device);VkQueryPool pool=reinterpret_cast<VkQueryPool>(query_pool);
    std::vector<VkPerformanceCounterResultKHR> raw(size_t(query_count)*counter_count);
    const VkDeviceSize stride=VkDeviceSize(sizeof(VkPerformanceCounterResultKHR))*counter_count;
    VkResult vr=vkGetQueryPoolResults(dev,pool,0,query_count,raw.size()*sizeof(raw[0]),raw.data(),stride,0);
    if(vr!=VK_SUCCESS){seterr(error,error_size,vrmsg("vkGetQueryPoolResults(perf)",vr));return 2;}
    for(uint32_t q=0;q<query_count;++q)for(uint32_t c=0;c<counter_count;++c){
        const auto& r=raw[size_t(q)*counter_count+c];auto& o=out_values[size_t(q)*counter_count+c];
        o={};o.storage=meta[c].storage;
        switch(meta[c].storage){
            case VK_PERFORMANCE_COUNTER_STORAGE_INT32_KHR:o.i64=r.int32;break;
            case VK_PERFORMANCE_COUNTER_STORAGE_INT64_KHR:o.i64=r.int64;break;
            case VK_PERFORMANCE_COUNTER_STORAGE_UINT32_KHR:o.u64=r.uint32;break;
            case VK_PERFORMANCE_COUNTER_STORAGE_UINT64_KHR:o.u64=r.uint64;break;
            case VK_PERFORMANCE_COUNTER_STORAGE_FLOAT32_KHR:o.f64=r.float32;break;
            case VK_PERFORMANCE_COUNTER_STORAGE_FLOAT64_KHR:o.f64=r.float64;break;
            default:seterr(error,error_size,"unsupported counter storage");return 3;
        }
    }
    return 0;
}
extern "C" void m3_perf_destroy_query_pool(void* device,void* query_pool){
    if(device&&query_pool)vkDestroyQueryPool(reinterpret_cast<VkDevice>(device),reinterpret_cast<VkQueryPool>(query_pool),nullptr);
}
