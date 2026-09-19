#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "gguf.h"
#include "tensor_store.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <filesystem>

using VkFlags = uint32_t;
using VkBool32 = uint32_t;
using VkDeviceSize = uint64_t;
using VkResult = int32_t;
using VkStructureType = int32_t;
using VkBufferUsageFlags = uint32_t;
using VkBufferCreateFlags = uint32_t;
using VkSharingMode = int32_t;
using VkMemoryPropertyFlags = uint32_t;
using VkQueueFlags = uint32_t;
using VkDeviceQueueCreateFlags = uint32_t;
using VkDeviceCreateFlags = uint32_t;
using VkInstanceCreateFlags = uint32_t;
using VkCommandPoolCreateFlags = uint32_t;
using VkCommandBufferUsageFlags = uint32_t;
using VkFenceCreateFlags = uint32_t;
using VkCommandBufferLevel = int32_t;
using VkShaderStageFlags = uint32_t;
using VkShaderStageFlagBits = uint32_t;
using VkDescriptorType = int32_t;
using VkDescriptorPoolCreateFlags = uint32_t;
using VkDescriptorSetLayoutCreateFlags = uint32_t;
using VkPipelineLayoutCreateFlags = uint32_t;
using VkShaderModuleCreateFlags = uint32_t;
using VkPipelineShaderStageCreateFlags = uint32_t;
using VkPipelineCreateFlags = uint32_t;
using VkPipelineBindPoint = int32_t;
using VkAccessFlags = uint32_t;
using VkPipelineStageFlags = uint32_t;
using VkDependencyFlags = uint32_t;

struct VkInstance_T; using VkInstance = VkInstance_T*;
struct VkPhysicalDevice_T; using VkPhysicalDevice = VkPhysicalDevice_T*;
struct VkDevice_T; using VkDevice = VkDevice_T*;
struct VkQueue_T; using VkQueue = VkQueue_T*;
struct VkBuffer_T; using VkBuffer = VkBuffer_T*;
struct VkDeviceMemory_T; using VkDeviceMemory = VkDeviceMemory_T*;
struct VkCommandPool_T; using VkCommandPool = VkCommandPool_T*;
struct VkCommandBuffer_T; using VkCommandBuffer = VkCommandBuffer_T*;
struct VkFence_T; using VkFence = VkFence_T*;
struct VkSemaphore_T; using VkSemaphore = VkSemaphore_T*;
struct VkShaderModule_T; using VkShaderModule = VkShaderModule_T*;
struct VkDescriptorSetLayout_T; using VkDescriptorSetLayout = VkDescriptorSetLayout_T*;
struct VkPipelineLayout_T; using VkPipelineLayout = VkPipelineLayout_T*;
struct VkPipeline_T; using VkPipeline = VkPipeline_T*;
struct VkDescriptorPool_T; using VkDescriptorPool = VkDescriptorPool_T*;
struct VkDescriptorSet_T; using VkDescriptorSet = VkDescriptorSet_T*;
struct VkPipelineCache_T; using VkPipelineCache = VkPipelineCache_T*;
struct VkSampler_T; using VkSampler = VkSampler_T*;
struct VkBufferView_T; using VkBufferView = VkBufferView_T*;

static constexpr VkResult VK_SUCCESS = 0;

static constexpr VkStructureType VK_STRUCTURE_TYPE_APPLICATION_INFO = 0;
static constexpr VkStructureType VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1;
static constexpr VkStructureType VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2;
static constexpr VkStructureType VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3;
static constexpr VkStructureType VK_STRUCTURE_TYPE_SUBMIT_INFO = 4;
static constexpr VkStructureType VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5;
static constexpr VkStructureType VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8;
static constexpr VkStructureType VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12;
static constexpr VkStructureType VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO = 16;
static constexpr VkStructureType VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO = 18;
static constexpr VkStructureType VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO = 30;
static constexpr VkStructureType VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO = 29;
static constexpr VkStructureType VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO = 32;
static constexpr VkStructureType VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO = 33;
static constexpr VkStructureType VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO = 34;
static constexpr VkStructureType VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET = 35;
static constexpr VkStructureType VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 39;
static constexpr VkStructureType VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 40;
static constexpr VkStructureType VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 42;
static constexpr VkStructureType VK_STRUCTURE_TYPE_MEMORY_BARRIER = 46;

static constexpr uint32_t VK_MAX_MEMORY_TYPES = 32;
static constexpr uint32_t VK_MAX_MEMORY_HEAPS = 16;

static constexpr VkQueueFlags VK_QUEUE_COMPUTE_BIT = 0x00000002;
static constexpr VkMemoryPropertyFlags VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001;
static constexpr VkMemoryPropertyFlags VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002;
static constexpr VkMemoryPropertyFlags VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004;
static constexpr VkMemoryPropertyFlags VK_MEMORY_PROPERTY_HOST_CACHED_BIT = 0x00000008;

static constexpr VkBufferUsageFlags VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020;
static constexpr VkSharingMode VK_SHARING_MODE_EXCLUSIVE = 0;

static constexpr VkCommandPoolCreateFlags VK_COMMAND_POOL_CREATE_TRANSIENT_BIT = 0x00000001;
static constexpr VkCommandBufferUsageFlags VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x00000001;
static constexpr VkCommandBufferLevel VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0;

static constexpr VkShaderStageFlagBits VK_SHADER_STAGE_COMPUTE_BIT = 0x00000020;
static constexpr VkDescriptorType VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7;
static constexpr VkPipelineBindPoint VK_PIPELINE_BIND_POINT_COMPUTE = 1;
static constexpr VkBool32 VK_TRUE = 1;
static constexpr VkAccessFlags VK_ACCESS_SHADER_READ_BIT = 0x00000020;
static constexpr VkAccessFlags VK_ACCESS_SHADER_WRITE_BIT = 0x00000040;
static constexpr VkAccessFlags VK_ACCESS_HOST_READ_BIT = 0x00002000;
static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800;
static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_HOST_BIT = 0x00004000;
static constexpr uint32_t VK_API_VERSION_1_2 = (1u << 22) | (2u << 12);

struct VkApplicationInfo {
    VkStructureType sType;
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
};

struct VkInstanceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkInstanceCreateFlags flags;
    const VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct VkExtent3D { uint32_t width, height, depth; };

struct VkQueueFamilyProperties {
    VkQueueFlags queueFlags;
    uint32_t queueCount;
    uint32_t timestampValidBits;
    VkExtent3D minImageTransferGranularity;
};

struct VkMemoryType {
    VkMemoryPropertyFlags propertyFlags;
    uint32_t heapIndex;
};

struct VkMemoryHeap {
    VkDeviceSize size;
    VkFlags flags;
};

struct VkPhysicalDeviceMemoryProperties {
    uint32_t memoryTypeCount;
    VkMemoryType memoryTypes[VK_MAX_MEMORY_TYPES];
    uint32_t memoryHeapCount;
    VkMemoryHeap memoryHeaps[VK_MAX_MEMORY_HEAPS];
};

struct VkDeviceQueueCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDeviceQueueCreateFlags flags;
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    const float* pQueuePriorities;
};

struct VkDeviceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDeviceCreateFlags flags;
    uint32_t queueCreateInfoCount;
    const VkDeviceQueueCreateInfo* pQueueCreateInfos;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
    const void* pEnabledFeatures;
};

struct VkBufferCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkBufferCreateFlags flags;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;
};

struct VkMemoryRequirements {
    VkDeviceSize size;
    VkDeviceSize alignment;
    uint32_t memoryTypeBits;
};

struct VkMemoryAllocateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDeviceSize allocationSize;
    uint32_t memoryTypeIndex;
};

struct VkCommandPoolCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkCommandPoolCreateFlags flags;
    uint32_t queueFamilyIndex;
};

struct VkCommandBufferAllocateInfo {
    VkStructureType sType;
    const void* pNext;
    VkCommandPool commandPool;
    VkCommandBufferLevel level;
    uint32_t commandBufferCount;
};

struct VkCommandBufferBeginInfo {
    VkStructureType sType;
    const void* pNext;
    VkCommandBufferUsageFlags flags;
    const void* pInheritanceInfo;
};

struct VkFenceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFenceCreateFlags flags;
};

struct VkSubmitInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore* pWaitSemaphores;
    const VkFlags* pWaitDstStageMask;
    uint32_t commandBufferCount;
    const VkCommandBuffer* pCommandBuffers;
    uint32_t signalSemaphoreCount;
    const VkSemaphore* pSignalSemaphores;
};

struct VkMemoryBarrier {
    VkStructureType sType;
    const void* pNext;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
};

struct VkDescriptorSetLayoutBinding {
    uint32_t binding;
    VkDescriptorType descriptorType;
    uint32_t descriptorCount;
    VkShaderStageFlags stageFlags;
    const VkSampler* pImmutableSamplers;
};

struct VkDescriptorSetLayoutCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorSetLayoutCreateFlags flags;
    uint32_t bindingCount;
    const VkDescriptorSetLayoutBinding* pBindings;
};

struct VkPushConstantRange {
    VkShaderStageFlags stageFlags;
    uint32_t offset;
    uint32_t size;
};

struct VkPipelineLayoutCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineLayoutCreateFlags flags;
    uint32_t setLayoutCount;
    const VkDescriptorSetLayout* pSetLayouts;
    uint32_t pushConstantRangeCount;
    const VkPushConstantRange* pPushConstantRanges;
};

struct VkShaderModuleCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkShaderModuleCreateFlags flags;
    size_t codeSize;
    const uint32_t* pCode;
};

struct VkSpecializationInfo;

struct VkPipelineShaderStageCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineShaderStageCreateFlags flags;
    VkShaderStageFlagBits stage;
    VkShaderModule module;
    const char* pName;
    const VkSpecializationInfo* pSpecializationInfo;
};

struct VkComputePipelineCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineCreateFlags flags;
    VkPipelineShaderStageCreateInfo stage;
    VkPipelineLayout layout;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
};

struct VkDescriptorPoolSize {
    VkDescriptorType type;
    uint32_t descriptorCount;
};

struct VkDescriptorPoolCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorPoolCreateFlags flags;
    uint32_t maxSets;
    uint32_t poolSizeCount;
    const VkDescriptorPoolSize* pPoolSizes;
};

struct VkDescriptorSetAllocateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorPool descriptorPool;
    uint32_t descriptorSetCount;
    const VkDescriptorSetLayout* pSetLayouts;
};

struct VkDescriptorBufferInfo {
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize range;
};

struct VkWriteDescriptorSet {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorSet dstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    uint32_t descriptorCount;
    VkDescriptorType descriptorType;
    const void* pImageInfo;
    const VkDescriptorBufferInfo* pBufferInfo;
    const VkBufferView* pTexelBufferView;
};

using PFN_vkVoidFunction = void (WINAPI*)();
using PFN_vkGetInstanceProcAddr = PFN_vkVoidFunction (WINAPI*)(VkInstance, const char*);
using PFN_vkGetDeviceProcAddr = PFN_vkVoidFunction (WINAPI*)(VkDevice, const char*);
using PFN_vkCreateInstance = VkResult (WINAPI*)(const VkInstanceCreateInfo*, const void*, VkInstance*);
using PFN_vkDestroyInstance = void (WINAPI*)(VkInstance, const void*);
using PFN_vkEnumeratePhysicalDevices = VkResult (WINAPI*)(VkInstance, uint32_t*, VkPhysicalDevice*);
using PFN_vkGetPhysicalDeviceQueueFamilyProperties = void (WINAPI*)(VkPhysicalDevice, uint32_t*, VkQueueFamilyProperties*);
using PFN_vkGetPhysicalDeviceMemoryProperties = void (WINAPI*)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*);
using PFN_vkCreateDevice = VkResult (WINAPI*)(VkPhysicalDevice, const VkDeviceCreateInfo*, const void*, VkDevice*);
using PFN_vkDestroyDevice = void (WINAPI*)(VkDevice, const void*);
using PFN_vkGetDeviceQueue = void (WINAPI*)(VkDevice, uint32_t, uint32_t, VkQueue*);
using PFN_vkCreateBuffer = VkResult (WINAPI*)(VkDevice, const VkBufferCreateInfo*, const void*, VkBuffer*);
using PFN_vkDestroyBuffer = void (WINAPI*)(VkDevice, VkBuffer, const void*);
using PFN_vkGetBufferMemoryRequirements = void (WINAPI*)(VkDevice, VkBuffer, VkMemoryRequirements*);
using PFN_vkAllocateMemory = VkResult (WINAPI*)(VkDevice, const VkMemoryAllocateInfo*, const void*, VkDeviceMemory*);
using PFN_vkFreeMemory = void (WINAPI*)(VkDevice, VkDeviceMemory, const void*);
using PFN_vkBindBufferMemory = VkResult (WINAPI*)(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize);
using PFN_vkMapMemory = VkResult (WINAPI*)(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkFlags, void**);
using PFN_vkUnmapMemory = void (WINAPI*)(VkDevice, VkDeviceMemory);
using PFN_vkCreateDescriptorSetLayout = VkResult (WINAPI*)(VkDevice, const VkDescriptorSetLayoutCreateInfo*, const void*, VkDescriptorSetLayout*);
using PFN_vkDestroyDescriptorSetLayout = void (WINAPI*)(VkDevice, VkDescriptorSetLayout, const void*);
using PFN_vkCreatePipelineLayout = VkResult (WINAPI*)(VkDevice, const VkPipelineLayoutCreateInfo*, const void*, VkPipelineLayout*);
using PFN_vkDestroyPipelineLayout = void (WINAPI*)(VkDevice, VkPipelineLayout, const void*);
using PFN_vkCreateShaderModule = VkResult (WINAPI*)(VkDevice, const VkShaderModuleCreateInfo*, const void*, VkShaderModule*);
using PFN_vkDestroyShaderModule = void (WINAPI*)(VkDevice, VkShaderModule, const void*);
using PFN_vkCreateComputePipelines = VkResult (WINAPI*)(VkDevice, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo*, const void*, VkPipeline*);
using PFN_vkDestroyPipeline = void (WINAPI*)(VkDevice, VkPipeline, const void*);
using PFN_vkCreateDescriptorPool = VkResult (WINAPI*)(VkDevice, const VkDescriptorPoolCreateInfo*, const void*, VkDescriptorPool*);
using PFN_vkDestroyDescriptorPool = void (WINAPI*)(VkDevice, VkDescriptorPool, const void*);
using PFN_vkAllocateDescriptorSets = VkResult (WINAPI*)(VkDevice, const VkDescriptorSetAllocateInfo*, VkDescriptorSet*);
using PFN_vkUpdateDescriptorSets = void (WINAPI*)(VkDevice, uint32_t, const VkWriteDescriptorSet*, uint32_t, const void*);
using PFN_vkCreateCommandPool = VkResult (WINAPI*)(VkDevice, const VkCommandPoolCreateInfo*, const void*, VkCommandPool*);
using PFN_vkDestroyCommandPool = void (WINAPI*)(VkDevice, VkCommandPool, const void*);
using PFN_vkAllocateCommandBuffers = VkResult (WINAPI*)(VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer*);
using PFN_vkBeginCommandBuffer = VkResult (WINAPI*)(VkCommandBuffer, const VkCommandBufferBeginInfo*);
using PFN_vkEndCommandBuffer = VkResult (WINAPI*)(VkCommandBuffer);
using PFN_vkCmdBindPipeline = void (WINAPI*)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline);
using PFN_vkCmdBindDescriptorSets = void (WINAPI*)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet*, uint32_t, const uint32_t*);
using PFN_vkCmdPushConstants = void (WINAPI*)(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void*);
using PFN_vkCmdDispatch = void (WINAPI*)(VkCommandBuffer, uint32_t, uint32_t, uint32_t);
using PFN_vkCmdPipelineBarrier = void (WINAPI*)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const void*, uint32_t, const void*);
using PFN_vkCreateFence = VkResult (WINAPI*)(VkDevice, const VkFenceCreateInfo*, const void*, VkFence*);
using PFN_vkDestroyFence = void (WINAPI*)(VkDevice, VkFence, const void*);
using PFN_vkQueueSubmit = VkResult (WINAPI*)(VkQueue, uint32_t, const VkSubmitInfo*, VkFence);
using PFN_vkWaitForFences = VkResult (WINAPI*)(VkDevice, uint32_t, const VkFence*, VkBool32, uint64_t);
using PFN_vkDeviceWaitIdle = VkResult (WINAPI*)(VkDevice);

template<typename T>
static T req(PFN_vkGetInstanceProcAddr gipa, VkInstance inst, const char* name) {
    auto p = gipa(inst, name);
    if (!p) throw std::runtime_error(std::string("missing Vulkan function: ") + name);
    return reinterpret_cast<T>(p);
}

template<typename T>
static T reqd(PFN_vkGetDeviceProcAddr gdpa, VkDevice dev, const char* name) {
    auto p = gdpa(dev, name);
    if (!p) throw std::runtime_error(std::string("missing Vulkan device function: ") + name);
    return reinterpret_cast<T>(p);
}

struct Buffer {
    VkBuffer buffer = nullptr;
    VkDeviceMemory memory = nullptr;
    uint64_t size = 0;
    uint64_t allocation_size = 0;
    void* mapped = nullptr;
};


namespace p8b {
constexpr uint32_t F32=0, Q4_K=12, Q6_K=14;
constexpr uint64_t QK_K=256, Q4_BYTES=144, Q6_BYTES=210;
constexpr uint64_t EXPECTED_FILE_BYTES=4683074048ull;
constexpr uint64_t ARENA_CAP=268435456ull;
constexpr uint64_t MAX_CTX=4096ull;
constexpr uint64_t PREFILL=512ull;
constexpr uint64_t USABLE=16374562816ull;
constexpr uint64_t EXPECTED_WEIGHT_BYTES=4677120000ull;
constexpr uint64_t EXPECTED_KV_BYTES=469762048ull;
constexpr uint64_t EXPECTED_WORK_BYTES=200888324ull;
constexpr uint64_t EXPECTED_TOTAL_BYTES=5347770372ull;

struct ArenaPlan { uint64_t start=0,end=0; };
struct SegmentPlan {
    std::string name;
    uint32_t ggml_type=0;
    uint32_t index=0;
    uint64_t row_start=0,row_count=0,row_bytes=0;
    uint64_t file_start=0,file_end=0;
    uint32_t arena=0;
    uint64_t arena_byte_base=0;
};

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

class VkResidency {
public:
    void init(){
        mod_=LoadLibraryA("vulkan-1.dll");
        if(!mod_)throw std::runtime_error("vulkan-1.dll not found");
        gipa_=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(mod_,"vkGetInstanceProcAddr"));
        if(!gipa_)throw std::runtime_error("vkGetInstanceProcAddr missing");
        auto create_instance=req<PFN_vkCreateInstance>(gipa_,nullptr,"vkCreateInstance");
        VkApplicationInfo ai{};ai.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;ai.pApplicationName="ArcLLM-P8B";ai.pEngineName="ArcLLM";ai.apiVersion=VK_API_VERSION_1_2;
        VkInstanceCreateInfo ici{};ici.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;ici.pApplicationInfo=&ai;
        VkResult vr=create_instance(&ici,nullptr,&instance_);
        if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateInstance failed: "+std::to_string(vr));

        destroy_instance_=req<PFN_vkDestroyInstance>(gipa_,instance_,"vkDestroyInstance");
        auto enum_phys=req<PFN_vkEnumeratePhysicalDevices>(gipa_,instance_,"vkEnumeratePhysicalDevices");
        auto get_qprops=req<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(gipa_,instance_,"vkGetPhysicalDeviceQueueFamilyProperties");
        auto get_mem=req<PFN_vkGetPhysicalDeviceMemoryProperties>(gipa_,instance_,"vkGetPhysicalDeviceMemoryProperties");
        auto create_device=req<PFN_vkCreateDevice>(gipa_,instance_,"vkCreateDevice");
        gdpa_=req<PFN_vkGetDeviceProcAddr>(gipa_,instance_,"vkGetDeviceProcAddr");

        uint32_t ndev=0;
        if(enum_phys(instance_,&ndev,nullptr)!=VK_SUCCESS||ndev==0)throw std::runtime_error("no Vulkan device");
        physical_device_count_=ndev;
        std::vector<VkPhysicalDevice> devs(ndev);
        if(enum_phys(instance_,&ndev,devs.data())!=VK_SUCCESS)throw std::runtime_error("vkEnumeratePhysicalDevices failed");
        phys_=devs[0];

        uint32_t nq=0;get_qprops(phys_,&nq,nullptr);
        std::vector<VkQueueFamilyProperties> qp(nq);get_qprops(phys_,&nq,qp.data());
        bool found=false;
        for(uint32_t i=0;i<nq;++i){
            if((qp[i].queueFlags&VK_QUEUE_COMPUTE_BIT)&&qp[i].queueCount>0){queue_family_=i;found=true;break;}
        }
        if(!found)throw std::runtime_error("no compute queue");
        get_mem(phys_,&mem_props_);

        float priority=1.0f;
        VkDeviceQueueCreateInfo qci{};qci.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;qci.queueFamilyIndex=queue_family_;qci.queueCount=1;qci.pQueuePriorities=&priority;
        VkDeviceCreateInfo dci{};dci.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;dci.queueCreateInfoCount=1;dci.pQueueCreateInfos=&qci;
        vr=create_device(phys_,&dci,nullptr,&device_);
        if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateDevice failed: "+std::to_string(vr));

        destroy_device_=reqd<PFN_vkDestroyDevice>(gdpa_,device_,"vkDestroyDevice");
        create_buffer_=reqd<PFN_vkCreateBuffer>(gdpa_,device_,"vkCreateBuffer");
        destroy_buffer_=reqd<PFN_vkDestroyBuffer>(gdpa_,device_,"vkDestroyBuffer");
        get_buffer_req_=reqd<PFN_vkGetBufferMemoryRequirements>(gdpa_,device_,"vkGetBufferMemoryRequirements");
        allocate_memory_=reqd<PFN_vkAllocateMemory>(gdpa_,device_,"vkAllocateMemory");
        free_memory_=reqd<PFN_vkFreeMemory>(gdpa_,device_,"vkFreeMemory");
        bind_buffer_memory_=reqd<PFN_vkBindBufferMemory>(gdpa_,device_,"vkBindBufferMemory");
        map_memory_=reqd<PFN_vkMapMemory>(gdpa_,device_,"vkMapMemory");
        unmap_memory_=reqd<PFN_vkUnmapMemory>(gdpa_,device_,"vkUnmapMemory");
        device_wait_idle_=reqd<PFN_vkDeviceWaitIdle>(gdpa_,device_,"vkDeviceWaitIdle");
    }
    ~VkResidency(){cleanup();}
    Buffer make_buffer(uint64_t size,const void* initial=nullptr){
        Buffer b;b.size=size;
        VkBufferCreateInfo bci{};bci.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;bci.size=size;bci.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;bci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        VkResult vr=create_buffer_(device_,&bci,nullptr,&b.buffer);
        if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateBuffer failed: "+std::to_string(vr));
        VkMemoryRequirements mr{};get_buffer_req_(device_,b.buffer,&mr);b.allocation_size=mr.size;
        int mt=find_memory_type(mr.memoryTypeBits);
        if(mt<0){destroy_buffer_(device_,b.buffer,nullptr);b.buffer=nullptr;throw std::runtime_error("no DEVICE_LOCAL|HOST_VISIBLE|HOST_COHERENT memory type");}
        if(memory_type_index_==UINT32_MAX){
            memory_type_index_=uint32_t(mt);
            memory_type_flags_=mem_props_.memoryTypes[mt].propertyFlags;
            memory_heap_index_=mem_props_.memoryTypes[mt].heapIndex;
            memory_heap_size_=mem_props_.memoryHeaps[memory_heap_index_].size;
        }else if(uint32_t(mt)!=memory_type_index_){
            destroy_buffer_(device_,b.buffer,nullptr);b.buffer=nullptr;
            throw std::runtime_error("memory type changed across P8-B allocations");
        }
        VkMemoryAllocateInfo mai{};mai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;mai.allocationSize=mr.size;mai.memoryTypeIndex=uint32_t(mt);
        vr=allocate_memory_(device_,&mai,nullptr,&b.memory);
        if(vr!=VK_SUCCESS){destroy_buffer_(device_,b.buffer,nullptr);b.buffer=nullptr;throw std::runtime_error("vkAllocateMemory failed: "+std::to_string(vr));}
        vr=bind_buffer_memory_(device_,b.buffer,b.memory,0);
        if(vr!=VK_SUCCESS){free_memory_(device_,b.memory,nullptr);destroy_buffer_(device_,b.buffer,nullptr);b={};throw std::runtime_error("vkBindBufferMemory failed: "+std::to_string(vr));}
        vr=map_memory_(device_,b.memory,0,size,0,&b.mapped);
        if(vr!=VK_SUCCESS||!b.mapped){free_memory_(device_,b.memory,nullptr);destroy_buffer_(device_,b.buffer,nullptr);b={};throw std::runtime_error("vkMapMemory failed: "+std::to_string(vr));}
        if(initial)std::memcpy(b.mapped,initial,size);else std::memset(b.mapped,0,size);
        return b;
    }
    void destroy_buffer(Buffer&b){
        if(b.mapped){unmap_memory_(device_,b.memory);b.mapped=nullptr;}
        if(b.buffer){destroy_buffer_(device_,b.buffer,nullptr);b.buffer=nullptr;}
        if(b.memory){free_memory_(device_,b.memory,nullptr);b.memory=nullptr;}
        b.size=0;b.allocation_size=0;
    }
    uint32_t physical_device_count()const{return physical_device_count_;}
    uint32_t queue_family()const{return queue_family_;}
    uint32_t memory_type_index()const{return memory_type_index_;}
    uint32_t memory_type_flags()const{return memory_type_flags_;}
    uint32_t memory_heap_index()const{return memory_heap_index_;}
    uint64_t memory_heap_size()const{return memory_heap_size_;}
private:
    int find_memory_type(uint32_t bits){
        const VkMemoryPropertyFlags required=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        int fallback=-1;
        for(uint32_t i=0;i<mem_props_.memoryTypeCount;++i){
            if(!(bits&(1u<<i)))continue;
            auto flags=mem_props_.memoryTypes[i].propertyFlags;
            if((flags&required)!=required)continue;
            if(flags&VK_MEMORY_PROPERTY_HOST_CACHED_BIT)return int(i);
            if(fallback<0)fallback=int(i);
        }
        return fallback;
    }
    void cleanup(){
        if(device_&&device_wait_idle_)device_wait_idle_(device_);
        if(device_&&destroy_device_)destroy_device_(device_,nullptr);
        device_=nullptr;
        if(instance_&&destroy_instance_)destroy_instance_(instance_,nullptr);
        instance_=nullptr;
        if(mod_)FreeLibrary(mod_);
        mod_=nullptr;
    }
    HMODULE mod_=nullptr;
    PFN_vkGetInstanceProcAddr gipa_=nullptr;
    PFN_vkGetDeviceProcAddr gdpa_=nullptr;
    VkInstance instance_=nullptr;
    VkPhysicalDevice phys_=nullptr;
    VkDevice device_=nullptr;
    uint32_t physical_device_count_=0,queue_family_=0;
    uint32_t memory_type_index_=UINT32_MAX,memory_type_flags_=0,memory_heap_index_=UINT32_MAX;
    uint64_t memory_heap_size_=0;
    VkPhysicalDeviceMemoryProperties mem_props_{};
    PFN_vkDestroyInstance destroy_instance_=nullptr;
    PFN_vkDestroyDevice destroy_device_=nullptr;
    PFN_vkCreateBuffer create_buffer_=nullptr;
    PFN_vkDestroyBuffer destroy_buffer_=nullptr;
    PFN_vkGetBufferMemoryRequirements get_buffer_req_=nullptr;
    PFN_vkAllocateMemory allocate_memory_=nullptr;
    PFN_vkFreeMemory free_memory_=nullptr;
    PFN_vkBindBufferMemory bind_buffer_memory_=nullptr;
    PFN_vkMapMemory map_memory_=nullptr;
    PFN_vkUnmapMemory unmap_memory_=nullptr;
    PFN_vkDeviceWaitIdle device_wait_idle_=nullptr;
};

std::vector<ArenaPlan> expected_arenas(){
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
std::vector<SegmentPlan> expected_segments(){
    return {
        {"token_embd.weight",Q4_K,0,0,133152,2016,0,268434432,0,0},
        {"token_embd.weight",Q4_K,1,133152,18912,2016,268434432,306561024,1,0},
        {"output.weight",Q6_K,0,0,91304,2940,4230051840,4498485600,17,0},
        {"output.weight",Q6_K,1,91304,60760,2940,4498485600,4677120000,18,0}
    };
}

std::vector<ArenaPlan> recompute_arenas(const GgufInfo&g){
    struct Piece{uint64_t start,end;};
    std::vector<const GgufTensorInfo*> ts;for(const auto&t:g.tensors)ts.push_back(&t);
    std::sort(ts.begin(),ts.end(),[](auto*a,auto*b){return a->offset<b->offset;});
    std::vector<Piece> pieces;
    uint64_t payload=0;
    for(auto*t:ts){
        uint64_t rb=row_bytes(t->ggml_type,t->dims.at(0)),rows=rows_of(*t),bytes=checked_mul(rb,rows);
        payload=(std::max)(payload,checked_add(t->offset,bytes));
        if(bytes<=ARENA_CAP){pieces.push_back({t->offset,t->offset+bytes});continue;}
        uint64_t max_rows=ARENA_CAP/rb,row=0;
        if(max_rows==0)throw std::runtime_error("row larger than arena cap");
        while(row<rows){
            uint64_t cnt=(std::min)(max_rows,rows-row);
            uint64_t s=checked_add(t->offset,checked_mul(row,rb)),e=checked_add(s,checked_mul(cnt,rb));
            pieces.push_back({s,e});row+=cnt;
        }
    }
    std::sort(pieces.begin(),pieces.end(),[](auto&a,auto&b){return a.start<b.start;});
    std::vector<ArenaPlan> out;uint64_t start=0;
    for(const auto&p:pieces){
        if(p.end-start>ARENA_CAP){if(p.start<=start)throw std::runtime_error("cannot pack piece");out.push_back({start,p.start});start=p.start;}
        if(p.end-start>ARENA_CAP)throw std::runtime_error("piece exceeds arena");
    }
    if(payload>start)out.push_back({start,payload});
    return out;
}
bool arenas_equal(const std::vector<ArenaPlan>&a,const std::vector<ArenaPlan>&b){
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(a[i].start!=b[i].start||a[i].end!=b[i].end)return false;
    return true;
}
bool validate_segment_metadata(const GgufInfo&g,const std::vector<SegmentPlan>&segs){
    for(const auto&s:segs){
        const auto*t=find_tensor(g,s.name);if(!t)return false;
        if(t->ggml_type!=s.ggml_type||t->dims.size()!=2||t->dims[1]!=152064)return false;
        if(row_bytes(t->ggml_type,t->dims[0])!=s.row_bytes)return false;
        if(s.file_start!=t->offset+s.row_start*s.row_bytes)return false;
        if(s.file_end!=s.file_start+s.row_count*s.row_bytes)return false;
    }
    return true;
}
bool exhaustive_row_translation(const GgufInfo&g,const std::vector<SegmentPlan>&segs,const std::vector<ArenaPlan>&arenas){
    for(const std::string name:{"token_embd.weight","output.weight"}){
        const auto*t=find_tensor(g,name);if(!t)return false;
        uint64_t rows=rows_of(*t),rb=row_bytes(t->ggml_type,t->dims[0]);
        for(uint64_t row=0;row<rows;++row){
            const SegmentPlan* hit=nullptr;
            for(const auto&s:segs)if(s.name==name&&row>=s.row_start&&row<s.row_start+s.row_count){hit=&s;break;}
            if(!hit||hit->arena>=arenas.size())return false;
            uint64_t local=row-hit->row_start;
            uint64_t resident_global=arenas[hit->arena].start+hit->arena_byte_base+local*rb;
            uint64_t source_global=t->offset+row*rb;
            if(resident_global!=source_global)return false;
        }
    }
    return true;
}
bool zero_sample(const Buffer&b){
    if(!b.mapped||b.size==0)return false;
    const uint8_t*p=static_cast<const uint8_t*>(b.mapped);
    uint64_t n=(std::min<uint64_t>)(64,b.size);
    for(uint64_t i=0;i<n;++i)if(p[i]!=0)return false;
    if(b.size>n)for(uint64_t i=b.size-n;i<b.size;++i)if(p[i]!=0)return false;
    return true;
}
}

int main(int argc,char**argv){
    using namespace p8b;
    std::string model,out="p8b_residency_results.json";
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="--model"&&i+1<argc)model=argv[++i];
        else if(a=="--out"&&i+1<argc)out=argv[++i];
        else{std::cerr<<"usage: arcllm_p8b --model <gguf> [--out <json>]\n";return 2;}
    }
    if(model.empty()){std::cerr<<"--model required\n";return 2;}

    auto write_error=[&](const std::string&stage,const std::string&msg,const std::string&status){
        std::ofstream o(out,std::ios::binary);
        if(o)o<<"{\n  \"schema\":\"arcllm.p8b.segmented_residency.v1\",\n  \"status\":\""<<status<<"\",\n  \"failure_stage\":\""<<esc(stage)<<"\",\n  \"error\":\""<<esc(msg)<<"\"\n}\n";
    };

    try{
        if(std::filesystem::file_size(std::filesystem::u8path(model))!=EXPECTED_FILE_BYTES)throw std::runtime_error("target size mismatch");
        GgufInfo g=GgufReader(model).read();
        TensorStore store;
        TensorStoreReport ts=store.inspect(model,g);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("TensorStore invariants failed: "+ts.error);
        if(ts.tensor_bytes_total!=EXPECTED_WEIGHT_BYTES)throw std::runtime_error("weight payload total mismatch");

        auto arenas_plan=recompute_arenas(g);
        auto frozen_arenas=expected_arenas();
        auto segs=expected_segments();
        bool exact_plan=arenas_equal(arenas_plan,frozen_arenas)&&validate_segment_metadata(g,segs);
        if(!exact_plan)throw std::runtime_error("P8-A2 frozen plan mismatch");
        bool row_translation=exhaustive_row_translation(g,segs,arenas_plan);
        if(!row_translation)throw std::runtime_error("segmented row translation mismatch");

        VkResidency vk;
        try{vk.init();}catch(const std::exception&e){write_error("vulkan_init",e.what(),"ERROR");std::cerr<<e.what()<<"\n";return 2;}

        std::vector<Buffer> weights,kv,work;
        auto destroy_all=[&](){
            for(auto&b:work)vk.destroy_buffer(b);
            for(auto&b:kv)vk.destroy_buffer(b);
            for(auto&b:weights)vk.destroy_buffer(b);
        };

        bool weight_alloc=false,weight_compare=false,kv_alloc=false,work_alloc=false,zero_pass=false;
        uint64_t requested=0,allocated=0,compared=0;
        std::string fail_stage,fail_message;
        auto add_counts=[&](const Buffer&b){requested=checked_add(requested,b.size);allocated=checked_add(allocated,b.allocation_size);};

        try{
            const uint8_t* payload=store.mapped_base()+g.data_offset;
            weights.reserve(arenas_plan.size());
            for(size_t i=0;i<arenas_plan.size();++i){
                uint64_t n=arenas_plan[i].end-arenas_plan[i].start;
                try{
                    weights.push_back(vk.make_buffer(n,payload+arenas_plan[i].start));
                }catch(const std::exception&e){
                    fail_stage="weight_arena_"+std::to_string(i);fail_message=e.what();throw;
                }
                add_counts(weights.back());
            }
            weight_alloc=weights.size()==19;
            weight_compare=weight_alloc;
            if(weight_compare){
                for(size_t i=0;i<weights.size();++i){
                    uint64_t n=arenas_plan[i].end-arenas_plan[i].start;
                    if(std::memcmp(weights[i].mapped,payload+arenas_plan[i].start,size_t(n))!=0){weight_compare=false;fail_stage="weight_compare_"+std::to_string(i);fail_message="full byte compare mismatch";break;}
                    compared=checked_add(compared,n);
                }
            }
            if(!weight_compare)throw std::runtime_error(fail_message);

            const uint64_t one_kv=checked_mul(checked_mul(checked_mul(28ull,MAX_CTX),512ull),4ull);
            if(checked_mul(one_kv,2)!=EXPECTED_KV_BYTES)throw std::runtime_error("KV formula mismatch");
            kv.reserve(2);
            for(uint32_t i=0;i<2;++i){
                try{kv.push_back(vk.make_buffer(one_kv));}
                catch(const std::exception&e){fail_stage="kv_"+std::to_string(i);fail_message=e.what();throw;}
                add_counts(kv.back());
            }
            kv_alloc=kv.size()==2;

            const uint64_t hidden_bytes=checked_mul(checked_mul(PREFILL,3584ull),4ull);
            const uint64_t kv_bytes=checked_mul(checked_mul(PREFILL,512ull),4ull);
            const uint64_t ffn_bytes=checked_mul(checked_mul(PREFILL,18944ull),4ull);
            auto alloc_work=[&](uint64_t n,const std::string&name){
                try{work.push_back(vk.make_buffer(n));}
                catch(const std::exception&e){fail_stage=name;fail_message=e.what();throw;}
                add_counts(work.back());
            };
            for(uint32_t i=0;i<11;++i)alloc_work(hidden_bytes,"work_hidden_"+std::to_string(i));
            for(uint32_t i=0;i<3;++i)alloc_work(kv_bytes,"work_kv_"+std::to_string(i));
            for(uint32_t i=0;i<3;++i)alloc_work(ffn_bytes,"work_ffn_"+std::to_string(i));
            alloc_work(152064ull*4ull,"work_logits");
            alloc_work(PREFILL*4ull,"work_prefill_ids");
            alloc_work(4ull,"work_decode_id");
            uint64_t work_requested=0;for(const auto&b:work)work_requested=checked_add(work_requested,b.size);
            work_alloc=work.size()==20&&work_requested==EXPECTED_WORK_BYTES;

            zero_pass=kv_alloc&&work_alloc;
            if(zero_pass){
                for(const auto&b:kv)if(!zero_sample(b)){zero_pass=false;break;}
                for(const auto&b:work)if(!zero_sample(b)){zero_pass=false;break;}
            }
        }catch(const std::exception&e){
            if(fail_stage.empty()){fail_stage="allocation_or_copy";fail_message=e.what();}
        }

        const bool memory_flags_pass=
            (vk.memory_type_flags()&(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
            (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        const bool requested_exact=requested==EXPECTED_TOTAL_BYTES;
        const bool allocated_budget_pass=allocated<=USABLE;
        const bool pass=fail_stage.empty()&&exact_plan&&row_translation&&memory_flags_pass&&weight_alloc&&weight_compare&&
                        compared==EXPECTED_WEIGHT_BYTES&&kv_alloc&&work_alloc&&zero_pass&&requested_exact&&allocated_budget_pass;

        std::ofstream o(out,std::ios::binary);
        if(!o){destroy_all();throw std::runtime_error("cannot write result JSON");}
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.p8b.segmented_residency.v1\",\n";
        o<<"  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";
        o<<"  \"target\":{\"size_bytes\":"<<EXPECTED_FILE_BYTES<<",\"tensor_count\":"<<g.tensor_count<<",\"data_offset\":"<<g.data_offset<<"},\n";
        o<<"  \"parent_plan\":{\"arena_count\":"<<arenas_plan.size()<<",\"segment_count\":4,\"exact_match\":"<<(exact_plan?"true":"false")<<",\"row_translation_exhaustive_pass\":"<<(row_translation?"true":"false")<<"},\n";
        o<<"  \"vulkan\":{\"physical_device_count\":"<<vk.physical_device_count()<<",\"queue_family\":"<<vk.queue_family()<<",\"memory_type_index\":"<<vk.memory_type_index()<<",\"memory_type_flags\":"<<vk.memory_type_flags()<<",\"memory_heap_index\":"<<vk.memory_heap_index()<<",\"memory_heap_size\":"<<vk.memory_heap_size()<<",\"required_memory_flags_pass\":"<<(memory_flags_pass?"true":"false")<<"},\n";
        o<<"  \"weights\":{\"arena_count\":"<<weights.size()<<",\"requested_bytes\":"<<EXPECTED_WEIGHT_BYTES<<",\"allocated_pass\":"<<(weight_alloc?"true":"false")<<",\"full_byte_compare_bytes\":"<<compared<<",\"full_byte_compare_pass\":"<<(weight_compare&&compared==EXPECTED_WEIGHT_BYTES?"true":"false")<<"},\n";
        o<<"  \"kv\":{\"buffer_count\":"<<kv.size()<<",\"requested_bytes\":"<<EXPECTED_KV_BYTES<<",\"allocation_pass\":"<<(kv_alloc?"true":"false")<<"},\n";
        o<<"  \"working\":{\"buffer_count\":"<<work.size()<<",\"requested_bytes\":"<<EXPECTED_WORK_BYTES<<",\"allocation_pass\":"<<(work_alloc?"true":"false")<<",\"zero_sample_pass\":"<<(zero_pass?"true":"false")<<"},\n";
        o<<"  \"residency\":{\"requested_bytes\":"<<requested<<",\"expected_requested_bytes\":"<<EXPECTED_TOTAL_BYTES<<",\"vulkan_allocation_bytes\":"<<allocated<<",\"usable_budget_bytes\":"<<USABLE<<",\"requested_exact_pass\":"<<(requested_exact?"true":"false")<<",\"allocation_budget_pass\":"<<(allocated_budget_pass?"true":"false")<<"},\n";
        o<<"  \"failure\":{\"stage\":\""<<esc(fail_stage)<<"\",\"message\":\""<<esc(fail_message)<<"\"},\n";
        o<<"  \"gate\":{\"tensor_store_pass\":true,\"parent_plan_match\":"<<(exact_plan?"true":"false")<<",\"row_translation_pass\":"<<(row_translation?"true":"false")<<",\"memory_flags_pass\":"<<(memory_flags_pass?"true":"false")<<",\"weight_allocation_pass\":"<<(weight_alloc?"true":"false")<<",\"weight_full_compare_pass\":"<<(weight_compare&&compared==EXPECTED_WEIGHT_BYTES?"true":"false")<<",\"kv_allocation_pass\":"<<(kv_alloc?"true":"false")<<",\"working_allocation_pass\":"<<(work_alloc?"true":"false")<<",\"requested_exact_pass\":"<<(requested_exact?"true":"false")<<",\"allocation_budget_pass\":"<<(allocated_budget_pass?"true":"false")<<",\"p8b_pass\":"<<(pass?"true":"false")<<"}\n";
        o<<"}\n";o.close();

        std::cout<<"ArcLLM P8-B segmented residency allocation/copy bring-up\n";
        std::cout<<"arenas="<<weights.size()<<" weight_bytes="<<EXPECTED_WEIGHT_BYTES<<" compared="<<compared<<"\n";
        std::cout<<"kv_buffers="<<kv.size()<<" kv_bytes="<<EXPECTED_KV_BYTES<<" work_buffers="<<work.size()<<" work_bytes="<<EXPECTED_WORK_BYTES<<"\n";
        std::cout<<"requested="<<requested<<" actual_vk_allocation="<<allocated<<" usable="<<USABLE<<"\n";
        std::cout<<"memory_type="<<vk.memory_type_index()<<" flags="<<vk.memory_type_flags()<<" heap="<<vk.memory_heap_index()<<" heap_size="<<vk.memory_heap_size()<<"\n";
        if(!fail_stage.empty())std::cout<<"failure_stage="<<fail_stage<<" message="<<fail_message<<"\n";
        std::cout<<"P8-B "<<(pass?"PASS":"FAIL")<<"\n";

        destroy_all();
        return pass?0:20;
    }catch(const std::exception&e){
        write_error("pre_residency",e.what(),"ERROR");
        std::cerr<<e.what()<<"\n";
        return 2;
    }
}
