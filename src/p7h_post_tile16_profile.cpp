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
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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
using VkQueryPoolCreateFlags = uint32_t;
using VkQueryType = int32_t;
using VkQueryResultFlags = uint32_t;

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
struct VkQueryPool_T; using VkQueryPool = VkQueryPool_T*;

static constexpr VkResult VK_SUCCESS = 0;

static constexpr VkStructureType VK_STRUCTURE_TYPE_APPLICATION_INFO = 0;
static constexpr VkStructureType VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1;
static constexpr VkStructureType VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2;
static constexpr VkStructureType VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3;
static constexpr VkStructureType VK_STRUCTURE_TYPE_SUBMIT_INFO = 4;
static constexpr VkStructureType VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5;
static constexpr VkStructureType VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8;
static constexpr VkStructureType VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO = 11;
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
static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001;
static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800;
static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000;
static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_HOST_BIT = 0x00004000;
static constexpr VkQueryType VK_QUERY_TYPE_TIMESTAMP = 2;
static constexpr VkQueryResultFlags VK_QUERY_RESULT_64_BIT = 0x00000001;
static constexpr VkQueryResultFlags VK_QUERY_RESULT_WAIT_BIT = 0x00000002;
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
struct VkQueryPoolCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkQueryPoolCreateFlags flags;
    VkQueryType queryType;
    uint32_t queryCount;
    VkFlags pipelineStatistics;
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
using PFN_vkCreateQueryPool = VkResult (WINAPI*)(VkDevice, const VkQueryPoolCreateInfo*, const void*, VkQueryPool*);
using PFN_vkDestroyQueryPool = void (WINAPI*)(VkDevice, VkQueryPool, const void*);
using PFN_vkCmdResetQueryPool = void (WINAPI*)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t);
using PFN_vkCmdWriteTimestamp = void (WINAPI*)(VkCommandBuffer, VkPipelineStageFlags, VkQueryPool, uint32_t);
using PFN_vkGetQueryPoolResults = VkResult (WINAPI*)(VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void*, VkDeviceSize, VkQueryResultFlags);

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
    void* mapped = nullptr;
};

struct DispatchOp {
    std::string name;
    std::string spv_path;
    std::vector<Buffer*> buffers;
    std::vector<uint8_t> push;
    uint32_t gx = 1;
    uint32_t gy = 1;
    uint32_t gz = 1;
};

struct ChainStats {
    uint32_t dispatch_count = 0;
    uint32_t submit_count = 0;
    uint32_t fence_wait_count = 0;
    uint32_t internal_barrier_count = 0;
    uint32_t final_host_barrier_count = 0;
    uint32_t initial_compute_barrier_count = 0;
    double record_submit_wait_ms = 0.0;
    double submit_wait_ms = 0.0;
};

struct ProfileStats {
    ChainStats chain;
    uint32_t timestamp_valid_bits = 0;
    uint64_t chain_ticks = 0;
    uint64_t dispatch_tick_sum = 0;
    uint64_t barrier_or_unattributed_ticks = 0;
    std::vector<uint64_t> op_ticks;
};

struct PreparedOp {
    VkDescriptorSetLayout set_layout = nullptr;
    VkPipelineLayout pipeline_layout = nullptr;
    VkShaderModule shader = nullptr;
    VkPipeline pipeline = nullptr;
    VkDescriptorPool descriptor_pool = nullptr;
    VkDescriptorSet descriptor_set = nullptr;
    std::string spv_path;
    std::vector<Buffer*> buffers;
    uint32_t push_size = 0;
};

struct PreparedChain {
    std::vector<PreparedOp> prepared;
    double prepare_ms = 0.0;
};

template<typename T>
static std::vector<uint8_t> push_bytes(const T& v) {
    std::vector<uint8_t> out(sizeof(T));
    std::memcpy(out.data(), &v, sizeof(T));
    return out;
}

struct Metrics {
    double max_abs = 0.0;
    double rmse = 0.0;
    bool pass = false;
};

static Metrics compare_vec(const std::vector<float>& ref, const std::vector<float>& got,
                           double max_abs_gate, double rmse_gate) {
    if (ref.size() != got.size() || ref.empty())
        throw std::runtime_error("compare_vec size mismatch");
    double max_abs = 0.0;
    double ss = 0.0;
    for (size_t i = 0; i < ref.size(); ++i) {
        double d = std::abs(double(ref[i]) - double(got[i]));
        max_abs = (std::max)(max_abs, d);
        ss += d * d;
    }
    Metrics m;
    m.max_abs = max_abs;
    m.rmse = std::sqrt(ss / double(ref.size()));
    m.pass = m.max_abs <= max_abs_gate && m.rmse <= rmse_gate;
    return m;
}

static float half_to_float(uint16_t h) {
    uint32_t sign = (uint32_t(h & 0x8000u)) << 16;
    uint32_t exp = (h >> 10) & 0x1fu;
    uint32_t mant = h & 0x03ffu;
    uint32_t out = 0;
    if (exp == 0) {
        if (mant == 0) {
            out = sign;
        } else {
            int e = -14;
            while ((mant & 0x0400u) == 0) {
                mant <<= 1;
                --e;
            }
            mant &= 0x03ffu;
            uint32_t e32 = uint32_t(e + 127);
            out = sign | (e32 << 23) | (mant << 13);
        }
    } else if (exp == 31) {
        out = sign | 0x7f800000u | (mant << 13);
    } else {
        out = sign | ((exp - 15u + 127u) << 23) | (mant << 13);
    }
    float f;
    std::memcpy(&f, &out, sizeof(f));
    return f;
}

static void scale_min_cpu(const uint8_t* block, int j, uint8_t& sc, uint8_t& mn) {
    const uint8_t* q = block + 4;
    if (j < 4) {
        sc = q[j] & 63;
        mn = q[j + 4] & 63;
    } else {
        sc = (q[j + 4] & 0xF) | ((q[j - 4] >> 6) << 4);
        mn = (q[j + 4] >> 4) | ((q[j] >> 6) << 4);
    }
}

static float q4k_dot_row(const uint8_t* row, const float* x, uint32_t n) {
    const uint32_t nblocks = n / 256u;
    float sum = 0.0f;
    for (uint32_t ib = 0; ib < nblocks; ++ib) {
        const uint8_t* b = row + ib * 144u;
        uint16_t hd = uint16_t(b[0]) | (uint16_t(b[1]) << 8);
        uint16_t hm = uint16_t(b[2]) | (uint16_t(b[3]) << 8);
        float d = half_to_float(hd);
        float dmin = half_to_float(hm);

        for (uint32_t k = 0; k < 256u; ++k) {
            int subgroup = int(k >> 5u);
            uint8_t sc = 0, mn = 0;
            scale_min_cpu(b, subgroup, sc, mn);

            uint32_t group64 = k >> 6u;
            uint32_t local64 = k & 63u;
            uint8_t qb = b[16u + group64 * 32u + (local64 & 31u)];
            uint8_t q = local64 < 32u ? (qb & 15u) : (qb >> 4u);

            float w = d * float(sc) * float(q) - dmin * float(mn);
            sum += w * x[ib * 256u + k];
        }
    }
    return sum;
}

static std::vector<uint32_t> read_spv(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("cannot open SPIR-V: " + path);
    std::streamsize n = f.tellg();
    if (n <= 0 || (n % 4) != 0)
        throw std::runtime_error("invalid SPIR-V size: " + path);
    f.seekg(0);
    std::vector<uint32_t> words(size_t(n) / 4u);
    f.read(reinterpret_cast<char*>(words.data()), n);
    if (!f) throw std::runtime_error("failed reading SPIR-V");
    if (words[0] != 0x07230203u)
        throw std::runtime_error("bad SPIR-V magic");
    return words;
}

class VkRuntime {
public:
    void init() {
        mod_ = LoadLibraryA("vulkan-1.dll");
        if (!mod_) throw std::runtime_error("vulkan-1.dll not found");
        gipa_ = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            GetProcAddress(mod_, "vkGetInstanceProcAddr"));
        if (!gipa_) throw std::runtime_error("vkGetInstanceProcAddr missing");

        auto create_instance = req<PFN_vkCreateInstance>(gipa_, nullptr, "vkCreateInstance");

        VkApplicationInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        ai.pApplicationName = "ArcLLM-P7D";
        ai.pEngineName = "ArcLLM";
        ai.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo ici{};
        ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ici.pApplicationInfo = &ai;

        VkResult vr = create_instance(&ici, nullptr, &instance_);
        if (vr != VK_SUCCESS) throw std::runtime_error("vkCreateInstance failed");

        destroy_instance_ = req<PFN_vkDestroyInstance>(gipa_, instance_, "vkDestroyInstance");
        auto enum_phys = req<PFN_vkEnumeratePhysicalDevices>(gipa_, instance_, "vkEnumeratePhysicalDevices");
        auto get_qprops = req<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(gipa_, instance_, "vkGetPhysicalDeviceQueueFamilyProperties");
        auto get_mem = req<PFN_vkGetPhysicalDeviceMemoryProperties>(gipa_, instance_, "vkGetPhysicalDeviceMemoryProperties");
        auto create_device = req<PFN_vkCreateDevice>(gipa_, instance_, "vkCreateDevice");
        gdpa_ = req<PFN_vkGetDeviceProcAddr>(gipa_, instance_, "vkGetDeviceProcAddr");

        uint32_t ndev = 0;
        if (enum_phys(instance_, &ndev, nullptr) != VK_SUCCESS || ndev == 0)
            throw std::runtime_error("no Vulkan device");
        physical_device_count_ = ndev;

        std::vector<VkPhysicalDevice> devs(ndev);
        if (enum_phys(instance_, &ndev, devs.data()) != VK_SUCCESS)
            throw std::runtime_error("vkEnumeratePhysicalDevices failed");
        phys_ = devs[0];

        uint32_t nq = 0;
        get_qprops(phys_, &nq, nullptr);
        std::vector<VkQueueFamilyProperties> qprops(nq);
        get_qprops(phys_, &nq, qprops.data());

        bool found = false;
        for (uint32_t i = 0; i < nq; ++i) {
            if ((qprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && qprops[i].queueCount > 0) {
                queue_family_ = i;
                timestamp_valid_bits_ = qprops[i].timestampValidBits;
                found = true;
                break;
            }
        }
        if (!found) throw std::runtime_error("no compute queue");

        get_mem(phys_, &mem_props_);

        float priority = 1.0f;
        VkDeviceQueueCreateInfo qci{};
        qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = queue_family_;
        qci.queueCount = 1;
        qci.pQueuePriorities = &priority;

        VkDeviceCreateInfo dci{};
        dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dci.queueCreateInfoCount = 1;
        dci.pQueueCreateInfos = &qci;

        vr = create_device(phys_, &dci, nullptr, &device_);
        if (vr != VK_SUCCESS) throw std::runtime_error("vkCreateDevice failed");

        destroy_device_ = reqd<PFN_vkDestroyDevice>(gdpa_, device_, "vkDestroyDevice");
        get_device_queue_ = reqd<PFN_vkGetDeviceQueue>(gdpa_, device_, "vkGetDeviceQueue");
        create_buffer_ = reqd<PFN_vkCreateBuffer>(gdpa_, device_, "vkCreateBuffer");
        destroy_buffer_ = reqd<PFN_vkDestroyBuffer>(gdpa_, device_, "vkDestroyBuffer");
        get_buffer_req_ = reqd<PFN_vkGetBufferMemoryRequirements>(gdpa_, device_, "vkGetBufferMemoryRequirements");
        allocate_memory_ = reqd<PFN_vkAllocateMemory>(gdpa_, device_, "vkAllocateMemory");
        free_memory_ = reqd<PFN_vkFreeMemory>(gdpa_, device_, "vkFreeMemory");
        bind_buffer_memory_ = reqd<PFN_vkBindBufferMemory>(gdpa_, device_, "vkBindBufferMemory");
        map_memory_ = reqd<PFN_vkMapMemory>(gdpa_, device_, "vkMapMemory");
        unmap_memory_ = reqd<PFN_vkUnmapMemory>(gdpa_, device_, "vkUnmapMemory");

        create_descriptor_set_layout_ = reqd<PFN_vkCreateDescriptorSetLayout>(gdpa_, device_, "vkCreateDescriptorSetLayout");
        destroy_descriptor_set_layout_ = reqd<PFN_vkDestroyDescriptorSetLayout>(gdpa_, device_, "vkDestroyDescriptorSetLayout");
        create_pipeline_layout_ = reqd<PFN_vkCreatePipelineLayout>(gdpa_, device_, "vkCreatePipelineLayout");
        destroy_pipeline_layout_ = reqd<PFN_vkDestroyPipelineLayout>(gdpa_, device_, "vkDestroyPipelineLayout");
        create_shader_module_ = reqd<PFN_vkCreateShaderModule>(gdpa_, device_, "vkCreateShaderModule");
        destroy_shader_module_ = reqd<PFN_vkDestroyShaderModule>(gdpa_, device_, "vkDestroyShaderModule");
        create_compute_pipelines_ = reqd<PFN_vkCreateComputePipelines>(gdpa_, device_, "vkCreateComputePipelines");
        destroy_pipeline_ = reqd<PFN_vkDestroyPipeline>(gdpa_, device_, "vkDestroyPipeline");
        create_descriptor_pool_ = reqd<PFN_vkCreateDescriptorPool>(gdpa_, device_, "vkCreateDescriptorPool");
        destroy_descriptor_pool_ = reqd<PFN_vkDestroyDescriptorPool>(gdpa_, device_, "vkDestroyDescriptorPool");
        allocate_descriptor_sets_ = reqd<PFN_vkAllocateDescriptorSets>(gdpa_, device_, "vkAllocateDescriptorSets");
        update_descriptor_sets_ = reqd<PFN_vkUpdateDescriptorSets>(gdpa_, device_, "vkUpdateDescriptorSets");

        create_command_pool_ = reqd<PFN_vkCreateCommandPool>(gdpa_, device_, "vkCreateCommandPool");
        destroy_command_pool_ = reqd<PFN_vkDestroyCommandPool>(gdpa_, device_, "vkDestroyCommandPool");
        allocate_command_buffers_ = reqd<PFN_vkAllocateCommandBuffers>(gdpa_, device_, "vkAllocateCommandBuffers");
        begin_command_buffer_ = reqd<PFN_vkBeginCommandBuffer>(gdpa_, device_, "vkBeginCommandBuffer");
        end_command_buffer_ = reqd<PFN_vkEndCommandBuffer>(gdpa_, device_, "vkEndCommandBuffer");
        cmd_bind_pipeline_ = reqd<PFN_vkCmdBindPipeline>(gdpa_, device_, "vkCmdBindPipeline");
        cmd_bind_descriptor_sets_ = reqd<PFN_vkCmdBindDescriptorSets>(gdpa_, device_, "vkCmdBindDescriptorSets");
        cmd_push_constants_ = reqd<PFN_vkCmdPushConstants>(gdpa_, device_, "vkCmdPushConstants");
        cmd_dispatch_ = reqd<PFN_vkCmdDispatch>(gdpa_, device_, "vkCmdDispatch");
        cmd_pipeline_barrier_ = reqd<PFN_vkCmdPipelineBarrier>(gdpa_, device_, "vkCmdPipelineBarrier");

        create_fence_ = reqd<PFN_vkCreateFence>(gdpa_, device_, "vkCreateFence");
        destroy_fence_ = reqd<PFN_vkDestroyFence>(gdpa_, device_, "vkDestroyFence");
        queue_submit_ = reqd<PFN_vkQueueSubmit>(gdpa_, device_, "vkQueueSubmit");
        wait_for_fences_ = reqd<PFN_vkWaitForFences>(gdpa_, device_, "vkWaitForFences");
        device_wait_idle_ = reqd<PFN_vkDeviceWaitIdle>(gdpa_, device_, "vkDeviceWaitIdle");
        create_query_pool_ = reqd<PFN_vkCreateQueryPool>(gdpa_, device_, "vkCreateQueryPool");
        destroy_query_pool_ = reqd<PFN_vkDestroyQueryPool>(gdpa_, device_, "vkDestroyQueryPool");
        cmd_reset_query_pool_ = reqd<PFN_vkCmdResetQueryPool>(gdpa_, device_, "vkCmdResetQueryPool");
        cmd_write_timestamp_ = reqd<PFN_vkCmdWriteTimestamp>(gdpa_, device_, "vkCmdWriteTimestamp");
        get_query_pool_results_ = reqd<PFN_vkGetQueryPoolResults>(gdpa_, device_, "vkGetQueryPoolResults");

        get_device_queue_(device_, queue_family_, 0, &queue_);
        if (!queue_) throw std::runtime_error("queue is null");
    }

    ~VkRuntime() { cleanup(); }

    uint32_t physical_device_count() const { return physical_device_count_; }
    uint32_t queue_family() const { return queue_family_; }
    uint32_t memory_type_index() const { return memory_type_index_; }
    uint32_t memory_type_flags() const { return memory_type_flags_; }
    uint32_t timestamp_valid_bits() const { return timestamp_valid_bits_; }

    Buffer make_buffer(uint64_t size, const void* initial = nullptr) {
        Buffer b;
        b.size = size;

        VkBufferCreateInfo bci{};
        bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size = size;
        bci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult vr = create_buffer_(device_, &bci, nullptr, &b.buffer);
        if (vr != VK_SUCCESS) throw std::runtime_error("vkCreateBuffer failed");

        VkMemoryRequirements mr{};
        get_buffer_req_(device_, b.buffer, &mr);

        int mt = find_memory_type(mr.memoryTypeBits);
        if (mt < 0) throw std::runtime_error("no device-local host-visible coherent memory type");

        if (memory_type_index_ == UINT32_MAX) {
            memory_type_index_ = uint32_t(mt);
            memory_type_flags_ = mem_props_.memoryTypes[mt].propertyFlags;
        }

        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = mr.size;
        mai.memoryTypeIndex = uint32_t(mt);

        vr = allocate_memory_(device_, &mai, nullptr, &b.memory);
        if (vr != VK_SUCCESS) throw std::runtime_error("vkAllocateMemory failed");

        vr = bind_buffer_memory_(device_, b.buffer, b.memory, 0);
        if (vr != VK_SUCCESS) throw std::runtime_error("vkBindBufferMemory failed");

        vr = map_memory_(device_, b.memory, 0, size, 0, &b.mapped);
        if (vr != VK_SUCCESS || !b.mapped)
            throw std::runtime_error("vkMapMemory failed");

        if (initial) std::memcpy(b.mapped, initial, size);
        else std::memset(b.mapped, 0, size);

        return b;
    }

    void destroy_buffer(Buffer& b) {
        if (b.mapped) {
            unmap_memory_(device_, b.memory);
            b.mapped = nullptr;
        }
        if (b.buffer) {
            destroy_buffer_(device_, b.buffer, nullptr);
            b.buffer = nullptr;
        }
        if (b.memory) {
            free_memory_(device_, b.memory, nullptr);
            b.memory = nullptr;
        }
    }

    PreparedChain prepare_chain(const std::vector<DispatchOp>& ops) {
        if (ops.empty()) throw std::runtime_error("empty dispatch chain");
        auto t0=std::chrono::steady_clock::now();
        PreparedChain chain; chain.prepared.resize(ops.size());
        for(size_t oi=0;oi<ops.size();++oi){const auto& op=ops[oi];auto& p=chain.prepared[oi];p.spv_path=op.spv_path;p.buffers=op.buffers;p.push_size=uint32_t(op.push.size());auto words=read_spv(op.spv_path);
            std::vector<VkDescriptorSetLayoutBinding> bindings(op.buffers.size());for(uint32_t i=0;i<uint32_t(bindings.size());++i){bindings[i].binding=i;bindings[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;bindings[i].descriptorCount=1;bindings[i].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;}
            VkDescriptorSetLayoutCreateInfo slci{};slci.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;slci.bindingCount=uint32_t(bindings.size());slci.pBindings=bindings.data();VkResult vr=create_descriptor_set_layout_(device_,&slci,nullptr,&p.set_layout);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateDescriptorSetLayout failed for "+op.name);
            VkPushConstantRange pcr{};pcr.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;pcr.offset=0;pcr.size=uint32_t(op.push.size());VkPipelineLayoutCreateInfo plci{};plci.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;plci.setLayoutCount=1;plci.pSetLayouts=&p.set_layout;plci.pushConstantRangeCount=op.push.empty()?0u:1u;plci.pPushConstantRanges=op.push.empty()?nullptr:&pcr;vr=create_pipeline_layout_(device_,&plci,nullptr,&p.pipeline_layout);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreatePipelineLayout failed for "+op.name);
            VkShaderModuleCreateInfo smci{};smci.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;smci.codeSize=words.size()*sizeof(uint32_t);smci.pCode=words.data();vr=create_shader_module_(device_,&smci,nullptr,&p.shader);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateShaderModule failed for "+op.name);
            VkPipelineShaderStageCreateInfo stage{};stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;stage.module=p.shader;stage.pName="main";VkComputePipelineCreateInfo cpci{};cpci.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;cpci.stage=stage;cpci.layout=p.pipeline_layout;cpci.basePipelineIndex=-1;vr=create_compute_pipelines_(device_,nullptr,1,&cpci,nullptr,&p.pipeline);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateComputePipelines failed for "+op.name);
            VkDescriptorPoolSize ps{};ps.type=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;ps.descriptorCount=uint32_t(op.buffers.size());VkDescriptorPoolCreateInfo dpci{};dpci.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;dpci.maxSets=1;dpci.poolSizeCount=1;dpci.pPoolSizes=&ps;vr=create_descriptor_pool_(device_,&dpci,nullptr,&p.descriptor_pool);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateDescriptorPool failed for "+op.name);
            VkDescriptorSetAllocateInfo dsai{};dsai.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;dsai.descriptorPool=p.descriptor_pool;dsai.descriptorSetCount=1;dsai.pSetLayouts=&p.set_layout;vr=allocate_descriptor_sets_(device_,&dsai,&p.descriptor_set);if(vr!=VK_SUCCESS)throw std::runtime_error("vkAllocateDescriptorSets failed for "+op.name);
            std::vector<VkDescriptorBufferInfo> infos(op.buffers.size());std::vector<VkWriteDescriptorSet> writes(op.buffers.size());for(uint32_t i=0;i<uint32_t(op.buffers.size());++i){infos[i].buffer=op.buffers[i]->buffer;infos[i].offset=0;infos[i].range=op.buffers[i]->size;writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=p.descriptor_set;writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[i].pBufferInfo=&infos[i];}update_descriptor_sets_(device_,uint32_t(writes.size()),writes.data(),0,nullptr);
        }
        chain.prepare_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();return chain;
    }

    ChainStats execute_prepared(const PreparedChain& chain,const std::vector<DispatchOp>& ops,bool cross_submit_compute_barrier=false){
        if(ops.size()!=chain.prepared.size()||ops.empty())throw std::runtime_error("prepared chain shape mismatch");for(size_t i=0;i<ops.size();++i){const auto& p=chain.prepared[i];if(ops[i].spv_path!=p.spv_path||ops[i].push.size()!=p.push_size||ops[i].buffers.size()!=p.buffers.size())throw std::runtime_error("prepared op signature mismatch: "+ops[i].name);for(size_t j=0;j<ops[i].buffers.size();++j)if(ops[i].buffers[j]!=p.buffers[j])throw std::runtime_error("prepared op buffer mismatch: "+ops[i].name);}
        auto tall0=std::chrono::steady_clock::now();VkCommandPoolCreateInfo pci{};pci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pci.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;pci.queueFamilyIndex=queue_family_;VkCommandPool pool=nullptr;VkResult vr=create_command_pool_(device_,&pci,nullptr,&pool);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateCommandPool failed P7B");VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer cb=nullptr;vr=allocate_command_buffers_(device_,&ai,&cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkAllocateCommandBuffers failed P7B");VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;vr=begin_command_buffer_(cb,&bi);if(vr!=VK_SUCCESS)throw std::runtime_error("vkBeginCommandBuffer failed P7B");ChainStats st{};
        if(cross_submit_compute_barrier){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);st.initial_compute_barrier_count=1;}
        for(size_t oi=0;oi<ops.size();++oi){const auto& op=ops[oi];const auto& p=chain.prepared[oi];cmd_bind_pipeline_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline);cmd_bind_descriptor_sets_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline_layout,0,1,&p.descriptor_set,0,nullptr);if(!op.push.empty())cmd_push_constants_(cb,p.pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,uint32_t(op.push.size()),op.push.data());cmd_dispatch_(cb,op.gx,op.gy,op.gz);++st.dispatch_count;if(oi+1<ops.size()){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);++st.internal_barrier_count;}}
        VkMemoryBarrier hb{};hb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;hb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;hb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&hb,0,nullptr,0,nullptr);st.final_host_barrier_count=1;vr=end_command_buffer_(cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkEndCommandBuffer failed P7B");VkFenceCreateInfo fci{};fci.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;VkFence fence=nullptr;vr=create_fence_(device_,&fci,nullptr,&fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateFence failed P7B");VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;si.commandBufferCount=1;si.pCommandBuffers=&cb;auto ts0=std::chrono::steady_clock::now();vr=queue_submit_(queue_,1,&si,fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkQueueSubmit failed P7B");st.submit_count=1;vr=wait_for_fences_(device_,1,&fence,VK_TRUE,(std::numeric_limits<uint64_t>::max)());if(vr!=VK_SUCCESS)throw std::runtime_error("vkWaitForFences failed P7B");st.fence_wait_count=1;auto ts1=std::chrono::steady_clock::now();st.submit_wait_ms=std::chrono::duration<double,std::milli>(ts1-ts0).count();destroy_fence_(device_,fence,nullptr);destroy_command_pool_(device_,pool,nullptr);st.record_submit_wait_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tall0).count();return st;
    }


    ProfileStats execute_profiled(const PreparedChain& chain,const std::vector<DispatchOp>& ops,bool cross_submit_compute_barrier=false){
        if(timestamp_valid_bits_==0)throw std::runtime_error("compute queue has timestampValidBits=0");
        if(ops.size()!=chain.prepared.size()||ops.empty())throw std::runtime_error("profiled prepared chain shape mismatch");
        for(size_t i=0;i<ops.size();++i){const auto& p=chain.prepared[i];if(ops[i].spv_path!=p.spv_path||ops[i].push.size()!=p.push_size||ops[i].buffers.size()!=p.buffers.size())throw std::runtime_error("profiled prepared op signature mismatch: "+ops[i].name);for(size_t j=0;j<ops[i].buffers.size();++j)if(ops[i].buffers[j]!=p.buffers[j])throw std::runtime_error("profiled prepared op buffer mismatch: "+ops[i].name);}
        auto tick_delta=[&](uint64_t a,uint64_t b)->uint64_t{if(timestamp_valid_bits_>=64u)return b-a;uint64_t mask=(uint64_t(1)<<timestamp_valid_bits_)-1u;return (b-a)&mask;};
        ProfileStats ps{};ps.timestamp_valid_bits=timestamp_valid_bits_;ps.op_ticks.resize(ops.size());auto tall0=std::chrono::steady_clock::now();
        VkCommandPoolCreateInfo pci{};pci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pci.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;pci.queueFamilyIndex=queue_family_;VkCommandPool pool=nullptr;VkResult vr=create_command_pool_(device_,&pci,nullptr,&pool);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateCommandPool failed P7B");
        VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer cb=nullptr;vr=allocate_command_buffers_(device_,&ai,&cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkAllocateCommandBuffers failed P7B");
        const uint32_t qcount=uint32_t(ops.size()*2u+2u);VkQueryPoolCreateInfo qci{};qci.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;qci.queryType=VK_QUERY_TYPE_TIMESTAMP;qci.queryCount=qcount;VkQueryPool qp=nullptr;vr=create_query_pool_(device_,&qci,nullptr,&qp);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateQueryPool failed P7B");
        VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;vr=begin_command_buffer_(cb,&bi);if(vr!=VK_SUCCESS)throw std::runtime_error("vkBeginCommandBuffer failed P7B");cmd_reset_query_pool_(cb,qp,0,qcount);cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,qp,0);
        if(cross_submit_compute_barrier){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);ps.chain.initial_compute_barrier_count=1;}
        for(size_t oi=0;oi<ops.size();++oi){const auto& op=ops[oi];const auto& p=chain.prepared[oi];uint32_t qb=1u+uint32_t(oi)*2u;cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,qp,qb);cmd_bind_pipeline_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline);cmd_bind_descriptor_sets_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline_layout,0,1,&p.descriptor_set,0,nullptr);if(!op.push.empty())cmd_push_constants_(cb,p.pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,uint32_t(op.push.size()),op.push.data());cmd_dispatch_(cb,op.gx,op.gy,op.gz);cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,qp,qb+1u);++ps.chain.dispatch_count;if(oi+1<ops.size()){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);++ps.chain.internal_barrier_count;}}
        VkMemoryBarrier hb{};hb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;hb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;hb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&hb,0,nullptr,0,nullptr);ps.chain.final_host_barrier_count=1;cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,qp,qcount-1u);vr=end_command_buffer_(cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkEndCommandBuffer failed P7B");
        VkFenceCreateInfo fci{};fci.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;VkFence fence=nullptr;vr=create_fence_(device_,&fci,nullptr,&fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateFence failed P7B");VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;si.commandBufferCount=1;si.pCommandBuffers=&cb;auto ts0=std::chrono::steady_clock::now();vr=queue_submit_(queue_,1,&si,fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkQueueSubmit failed P7B");ps.chain.submit_count=1;vr=wait_for_fences_(device_,1,&fence,VK_TRUE,(std::numeric_limits<uint64_t>::max)());if(vr!=VK_SUCCESS)throw std::runtime_error("vkWaitForFences failed P7B");ps.chain.fence_wait_count=1;auto ts1=std::chrono::steady_clock::now();ps.chain.submit_wait_ms=std::chrono::duration<double,std::milli>(ts1-ts0).count();
        std::vector<uint64_t> ticks(qcount);vr=get_query_pool_results_(device_,qp,0,qcount,ticks.size()*sizeof(uint64_t),ticks.data(),sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT);if(vr!=VK_SUCCESS)throw std::runtime_error("vkGetQueryPoolResults failed P7B");ps.chain_ticks=tick_delta(ticks[0],ticks[qcount-1u]);for(size_t oi=0;oi<ops.size();++oi){uint32_t qb=1u+uint32_t(oi)*2u;uint64_t d=tick_delta(ticks[qb],ticks[qb+1u]);ps.op_ticks[oi]=d;ps.dispatch_tick_sum+=d;}ps.barrier_or_unattributed_ticks=(ps.chain_ticks>ps.dispatch_tick_sum)?(ps.chain_ticks-ps.dispatch_tick_sum):0;
        destroy_fence_(device_,fence,nullptr);destroy_query_pool_(device_,qp,nullptr);destroy_command_pool_(device_,pool,nullptr);ps.chain.record_submit_wait_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tall0).count();return ps;
    }

    void destroy_prepared(PreparedChain& chain){for(auto& p:chain.prepared){if(p.descriptor_pool)destroy_descriptor_pool_(device_,p.descriptor_pool,nullptr);if(p.pipeline)destroy_pipeline_(device_,p.pipeline,nullptr);if(p.shader)destroy_shader_module_(device_,p.shader,nullptr);if(p.pipeline_layout)destroy_pipeline_layout_(device_,p.pipeline_layout,nullptr);if(p.set_layout)destroy_descriptor_set_layout_(device_,p.set_layout,nullptr);}chain.prepared.clear();}

private:
    int find_memory_type(uint32_t bits) {
        const VkMemoryPropertyFlags required =
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        int fallback = -1;
        for (uint32_t i = 0; i < mem_props_.memoryTypeCount; ++i) {
            if (!(bits & (1u << i))) continue;
            auto flags = mem_props_.memoryTypes[i].propertyFlags;
            if ((flags & required) != required) continue;
            if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) return int(i);
            if (fallback < 0) fallback = int(i);
        }
        return fallback;
    }

    void cleanup() {
        if (device_ && device_wait_idle_) device_wait_idle_(device_);
        if (device_ && destroy_device_) destroy_device_(device_, nullptr);
        device_ = nullptr;
        if (instance_ && destroy_instance_) destroy_instance_(instance_, nullptr);
        instance_ = nullptr;
        if (mod_) FreeLibrary(mod_);
        mod_ = nullptr;
    }

    HMODULE mod_ = nullptr;
    PFN_vkGetInstanceProcAddr gipa_ = nullptr;
    PFN_vkGetDeviceProcAddr gdpa_ = nullptr;
    VkInstance instance_ = nullptr;
    VkPhysicalDevice phys_ = nullptr;
    VkDevice device_ = nullptr;
    VkQueue queue_ = nullptr;
    uint32_t physical_device_count_ = 0;
    uint32_t queue_family_ = 0;
    uint32_t memory_type_index_ = UINT32_MAX;
    uint32_t memory_type_flags_ = 0;
    uint32_t timestamp_valid_bits_ = 0;
    VkPhysicalDeviceMemoryProperties mem_props_{};

    PFN_vkDestroyInstance destroy_instance_ = nullptr;
    PFN_vkDestroyDevice destroy_device_ = nullptr;
    PFN_vkGetDeviceQueue get_device_queue_ = nullptr;
    PFN_vkCreateBuffer create_buffer_ = nullptr;
    PFN_vkDestroyBuffer destroy_buffer_ = nullptr;
    PFN_vkGetBufferMemoryRequirements get_buffer_req_ = nullptr;
    PFN_vkAllocateMemory allocate_memory_ = nullptr;
    PFN_vkFreeMemory free_memory_ = nullptr;
    PFN_vkBindBufferMemory bind_buffer_memory_ = nullptr;
    PFN_vkMapMemory map_memory_ = nullptr;
    PFN_vkUnmapMemory unmap_memory_ = nullptr;

    PFN_vkCreateDescriptorSetLayout create_descriptor_set_layout_ = nullptr;
    PFN_vkDestroyDescriptorSetLayout destroy_descriptor_set_layout_ = nullptr;
    PFN_vkCreatePipelineLayout create_pipeline_layout_ = nullptr;
    PFN_vkDestroyPipelineLayout destroy_pipeline_layout_ = nullptr;
    PFN_vkCreateShaderModule create_shader_module_ = nullptr;
    PFN_vkDestroyShaderModule destroy_shader_module_ = nullptr;
    PFN_vkCreateComputePipelines create_compute_pipelines_ = nullptr;
    PFN_vkDestroyPipeline destroy_pipeline_ = nullptr;
    PFN_vkCreateDescriptorPool create_descriptor_pool_ = nullptr;
    PFN_vkDestroyDescriptorPool destroy_descriptor_pool_ = nullptr;
    PFN_vkAllocateDescriptorSets allocate_descriptor_sets_ = nullptr;
    PFN_vkUpdateDescriptorSets update_descriptor_sets_ = nullptr;

    PFN_vkCreateCommandPool create_command_pool_ = nullptr;
    PFN_vkDestroyCommandPool destroy_command_pool_ = nullptr;
    PFN_vkAllocateCommandBuffers allocate_command_buffers_ = nullptr;
    PFN_vkBeginCommandBuffer begin_command_buffer_ = nullptr;
    PFN_vkEndCommandBuffer end_command_buffer_ = nullptr;
    PFN_vkCmdBindPipeline cmd_bind_pipeline_ = nullptr;
    PFN_vkCmdBindDescriptorSets cmd_bind_descriptor_sets_ = nullptr;
    PFN_vkCmdPushConstants cmd_push_constants_ = nullptr;
    PFN_vkCmdDispatch cmd_dispatch_ = nullptr;
    PFN_vkCmdPipelineBarrier cmd_pipeline_barrier_ = nullptr;
    PFN_vkCreateFence create_fence_ = nullptr;
    PFN_vkDestroyFence destroy_fence_ = nullptr;
    PFN_vkQueueSubmit queue_submit_ = nullptr;
    PFN_vkWaitForFences wait_for_fences_ = nullptr;
    PFN_vkDeviceWaitIdle device_wait_idle_ = nullptr;
    PFN_vkCreateQueryPool create_query_pool_ = nullptr;
    PFN_vkDestroyQueryPool destroy_query_pool_ = nullptr;
    PFN_vkCmdResetQueryPool cmd_reset_query_pool_ = nullptr;
    PFN_vkCmdWriteTimestamp cmd_write_timestamp_ = nullptr;
    PFN_vkGetQueryPoolResults get_query_pool_results_ = nullptr;
};

static const GgufTensorInfo* find_tensor(const GgufInfo& g, const std::string& name) {
    for (const auto& t : g.tensors) if (t.name == name) return &t;
    return nullptr;
}

static std::string json_bool(bool v) { return v ? "true" : "false"; }


static const uint8_t* tensor_ptr(const TensorStore& store, const GgufInfo& gguf,
                                 const GgufTensorInfo* t) {
    return store.mapped_base() + gguf.data_offset + t->offset;
}

static uint32_t q4_row_bytes(uint32_t n) {
    if (n % 256u) throw std::runtime_error("Q4_K row width is not a multiple of 256");
    return (n / 256u) * 144u;
}

static uint32_t q6_row_bytes(uint32_t n) {
    if (n % 256u) throw std::runtime_error("Q6_K row width is not a multiple of 256");
    return (n / 256u) * 210u;
}

static int q6_scale_signed(uint8_t v) {
    return v >= 128u ? int(v) - 256 : int(v);
}

static int q6_value_cpu(const uint8_t* b, uint32_t k, int& scale) {
    uint32_t half128 = k >> 7u;
    uint32_t kk = k & 127u;
    uint32_t l = kk & 31u;
    uint32_t quarter = kk >> 5u;
    const uint8_t* ql = b + half128 * 64u;
    const uint8_t* qh = b + 128u + half128 * 32u;
    const uint8_t* sc = b + 192u + half128 * 8u;
    uint32_t is = l >> 4u;
    uint32_t low = 0, high = 0, sc_idx = 0;
    if (quarter == 0u) {
        low = ql[l] & 15u; high = (qh[l] >> 0u) & 3u; sc_idx = is + 0u;
    } else if (quarter == 1u) {
        low = ql[l + 32u] & 15u; high = (qh[l] >> 2u) & 3u; sc_idx = is + 2u;
    } else if (quarter == 2u) {
        low = ql[l] >> 4u; high = (qh[l] >> 4u) & 3u; sc_idx = is + 4u;
    } else {
        low = ql[l + 32u] >> 4u; high = (qh[l] >> 6u) & 3u; sc_idx = is + 6u;
    }
    scale = q6_scale_signed(sc[sc_idx]);
    return int(low | (high << 4u)) - 32;
}

static float q6k_dot_row(const uint8_t* row, const float* x, uint32_t n) {
    float sum = 0.0f;
    uint32_t nb = n / 256u;
    for (uint32_t ib = 0; ib < nb; ++ib) {
        const uint8_t* b = row + uint64_t(ib) * 210u;
        uint16_t hd = uint16_t(b[208]) | (uint16_t(b[209]) << 8);
        float d = half_to_float(hd);
        for (uint32_t k = 0; k < 256u; ++k) {
            int sc = 0;
            int q = q6_value_cpu(b, k, sc);
            sum += (d * float(sc) * float(q)) * x[ib * 256u + k];
        }
    }
    return sum;
}

static void q6k_dequant_row(const uint8_t* row, float* y, uint32_t n) {
    uint32_t nb = n / 256u;
    for (uint32_t ib = 0; ib < nb; ++ib) {
        const uint8_t* b = row + uint64_t(ib) * 210u;
        uint16_t hd = uint16_t(b[208]) | (uint16_t(b[209]) << 8);
        float d = half_to_float(hd);
        for (uint32_t k = 0; k < 256u; ++k) {
            int sc = 0;
            int q = q6_value_cpu(b, k, sc);
            y[ib * 256u + k] = d * float(sc) * float(q);
        }
    }
}

static std::vector<float> rmsnorm_cpu(const std::vector<float>& x, const float* w,
                                      uint32_t batch, uint32_t n, float eps) {
    std::vector<float> y(x.size());
    for (uint32_t b = 0; b < batch; ++b) {
        double ss = 0.0;
        const float* xb = x.data() + size_t(b) * n;
        for (uint32_t i = 0; i < n; ++i) ss += double(xb[i]) * double(xb[i]);
        float inv = 1.0f / std::sqrt(float(ss / double(n)) + eps);
        for (uint32_t i = 0; i < n; ++i) y[size_t(b) * n + i] = xb[i] * inv * w[i];
    }
    return y;
}

static std::vector<float> matmul_q4_cpu(const uint8_t* w, uint32_t n, uint32_t rows,
                                        uint32_t batch, const std::vector<float>& x,
                                        const float* bias) {
    uint32_t rb = q4_row_bytes(n);
    std::vector<float> y(size_t(batch) * rows);
    for (uint32_t b = 0; b < batch; ++b) {
        const float* xb = x.data() + size_t(b) * n;
        for (uint32_t r = 0; r < rows; ++r) {
            float v = q4k_dot_row(w + uint64_t(r) * rb, xb, n);
            y[size_t(b) * rows + r] = v + (bias ? bias[r] : 0.0f);
        }
    }
    return y;
}

static std::vector<float> matmul_q6_cpu(const uint8_t* w, uint32_t n, uint32_t rows,
                                        uint32_t batch, const std::vector<float>& x,
                                        const float* bias) {
    uint32_t rb = q6_row_bytes(n);
    std::vector<float> y(size_t(batch) * rows);
    for (uint32_t b = 0; b < batch; ++b) {
        const float* xb = x.data() + size_t(b) * n;
        for (uint32_t r = 0; r < rows; ++r) {
            float v = q6k_dot_row(w + uint64_t(r) * rb, xb, n);
            y[size_t(b) * rows + r] = v + (bias ? bias[r] : 0.0f);
        }
    }
    return y;
}

static void rope_cpu(std::vector<float>& x, uint32_t batch, uint32_t heads,
                     uint32_t dim, uint32_t pos_base, float theta) {
    for (uint32_t b = 0; b < batch; ++b) {
        uint32_t pos = pos_base + b;
        for (uint32_t h = 0; h < heads; ++h) {
            size_t base = (size_t(b) * heads + h) * dim;
            for (uint32_t p = 0; p < dim / 2u; ++p) {
                size_t i0 = base + 2u * p;
                size_t i1 = i0 + 1u;
                float angle = float(pos) * std::pow(theta, -2.0f * float(p) / float(dim));
                float c = std::cos(angle), s = std::sin(angle);
                float a = x[i0], bb = x[i1];
                x[i0] = a * c - bb * s;
                x[i1] = a * s + bb * c;
            }
        }
    }
}

static std::vector<float> attention_cpu(const std::vector<float>& q,
                                        const std::vector<float>& k,
                                        const std::vector<float>& v,
                                        uint32_t seq, uint32_t qh, uint32_t kvh,
                                        uint32_t dim) {
    std::vector<float> out(size_t(seq) * qh * dim, 0.0f);
    uint32_t hpk = qh / kvh;
    double scale = 1.0 / std::sqrt(double(dim));
    for (uint32_t t = 0; t < seq; ++t) {
        for (uint32_t h = 0; h < qh; ++h) {
            uint32_t kh = h / hpk;
            std::vector<double> logits(t + 1u);
            for (uint32_t s = 0; s <= t; ++s) {
                double dot = 0.0;
                for (uint32_t d = 0; d < dim; ++d) {
                    dot += double(q[(size_t(t) * qh + h) * dim + d]) *
                           double(k[(size_t(s) * kvh + kh) * dim + d]);
                }
                logits[s] = dot * scale;
            }
            double mx = logits[0];
            for (double z : logits) mx = (std::max)(mx, z);
            double sm = 0.0;
            for (double& z : logits) { z = std::exp(z - mx); sm += z; }
            for (double& z : logits) z /= sm;
            for (uint32_t d = 0; d < dim; ++d) {
                double acc = 0.0;
                for (uint32_t s = 0; s <= t; ++s) {
                    acc += logits[s] * double(v[(size_t(s) * kvh + kh) * dim + d]);
                }
                out[(size_t(t) * qh + h) * dim + d] = float(acc);
            }
        }
    }
    return out;
}

static std::vector<float> add_cpu(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) throw std::runtime_error("add_cpu size mismatch");
    std::vector<float> y(a.size());
    for (size_t i = 0; i < a.size(); ++i) y[i] = a[i] + b[i];
    return y;
}

static std::vector<float> swiglu_cpu(const std::vector<float>& gate,
                                     const std::vector<float>& up) {
    if (gate.size() != up.size()) throw std::runtime_error("swiglu_cpu size mismatch");
    std::vector<float> y(gate.size());
    for (size_t i = 0; i < y.size(); ++i) {
        float g = gate[i];
        y[i] = (g / (1.0f + std::exp(-g))) * up[i];
    }
    return y;
}

static void require_dims(const GgufTensorInfo* t, uint32_t type,
                         uint64_t d0, uint64_t d1, const char* label) {
    if (!t || t->ggml_type != type || t->dims.size() != 2 ||
        t->dims[0] != d0 || t->dims[1] != d1) {
        throw std::runtime_error(std::string("tensor contract mismatch: ") + label);
    }
}

static void require_vec(const GgufTensorInfo* t, uint32_t type,
                        uint64_t d0, const char* label) {
    if (!t || t->ggml_type != type || t->dims.size() != 1 || t->dims[0] != d0)
        throw std::runtime_error(std::string("tensor contract mismatch: ") + label);
}


static uint64_t tensor_nbytes(const GgufTensorInfo& t) {
    uint64_t n = 1;
    for (uint64_t d : t.dims) n *= d;
    if (t.ggml_type == 0u) return n * 4u;
    if (t.ggml_type == 12u) { if (n % 256u) throw std::runtime_error("bad Q4_K tensor elements"); return (n/256u)*144u; }
    if (t.ggml_type == 14u) { if (n % 256u) throw std::runtime_error("bad Q6_K tensor elements"); return (n/256u)*210u; }
    throw std::runtime_error("unsupported tensor type in P5 residency");
}

struct TensorLoc { uint32_t arena = 0; uint32_t byte_base = 0; uint64_t bytes = 0; };
struct ArenaPlan { uint64_t start = 0; uint64_t end = 0; };

static std::vector<ArenaPlan> build_tensor_aware_arenas(const GgufInfo& gguf, uint64_t payload_bytes) {
    const uint64_t max_arena = 256ull * 1024ull * 1024ull;
    std::vector<const GgufTensorInfo*> ts;
    ts.reserve(gguf.tensors.size());
    for (const auto& t : gguf.tensors) ts.push_back(&t);
    std::sort(ts.begin(), ts.end(), [](auto a, auto b){ return a->offset < b->offset; });
    std::vector<ArenaPlan> a;
    uint64_t start = 0;
    for (auto* t : ts) {
        uint64_t tend = t->offset + tensor_nbytes(*t);
        if (tend > payload_bytes) throw std::runtime_error("tensor beyond payload");
        if (tend - start > max_arena && t->offset > start) {
            a.push_back({start, t->offset});
            start = t->offset;
        }
        if (tend - start > max_arena) throw std::runtime_error("single tensor exceeds 256 MiB arena");
    }
    a.push_back({start, payload_bytes});
    for (const auto& x : a) if (x.end <= x.start || x.end - x.start > max_arena)
        throw std::runtime_error("invalid arena plan");
    return a;
}

static uint32_t argmax(const std::vector<float>& v) {
    if (v.empty()) throw std::runtime_error("argmax empty");
    uint32_t best=0; for(uint32_t i=1;i<uint32_t(v.size());++i) if(v[i]>v[best]) best=i; return best;
}

static void require_quant_dims(const GgufTensorInfo* t, uint64_t d0, uint64_t d1, const char* label) {
    if (!t || (t->ggml_type != 12u && t->ggml_type != 14u) || t->dims.size()!=2 || t->dims[0]!=d0 || t->dims[1]!=d1)
        throw std::runtime_error(std::string("mixed quant tensor contract mismatch: ")+label);
}


static std::vector<float> attention_cache_cpu(const std::vector<float>& q,
                                               const std::vector<float>& kc,
                                               const std::vector<float>& vc,
                                               uint32_t layer, uint32_t pos, uint32_t max_ctx,
                                               uint32_t q_heads, uint32_t kv_heads, uint32_t dim) {
    if (q.size() != uint64_t(q_heads)*dim || pos >= max_ctx) throw std::runtime_error("attention_cache_cpu shape mismatch");
    std::vector<float> out(uint64_t(q_heads)*dim, 0.0f);
    uint32_t heads_per_kv = q_heads / kv_heads;
    float scale = 1.0f/std::sqrt(float(dim));
    for (uint32_t qh=0; qh<q_heads; ++qh) {
        uint32_t kvh=qh/heads_per_kv;
        std::vector<float> scores(pos+1u);
        float mx=-std::numeric_limits<float>::infinity();
        for(uint32_t t=0;t<=pos;++t){
            double s=0.0; uint64_t kb=((uint64_t(layer)*max_ctx+t)*kv_heads+kvh)*dim;
            for(uint32_t d=0;d<dim;++d) s+=double(q[uint64_t(qh)*dim+d])*double(kc[kb+d]);
            scores[t]=float(s*scale); mx=(std::max)(mx,scores[t]);
        }
        double sm=0.0; for(float& x:scores){x=std::exp(x-mx);sm+=x;} for(float& x:scores)x=float(x/sm);
        for(uint32_t d=0;d<dim;++d){double a=0.0;for(uint32_t t=0;t<=pos;++t){uint64_t vb=((uint64_t(layer)*max_ctx+t)*kv_heads+kvh)*dim+d;a+=double(scores[t])*double(vc[vb]);}out[uint64_t(qh)*dim+d]=float(a);}
    }
    return out;
}


static double median_of(std::vector<double> v){if(v.empty())return 0.0;std::sort(v.begin(),v.end());size_t n=v.size();return(n&1u)?v[n/2]:(v[n/2-1]+v[n/2])*0.5;}
static double percentile95(std::vector<double> v){if(v.empty())return 0.0;std::sort(v.begin(),v.end());size_t i=size_t(std::ceil(0.95*double(v.size())))-1u;if(i>=v.size())i=v.size()-1u;return v[i];}
static uint32_t argmax_ptr(const float* p,uint32_t n,bool& finite){uint32_t bi=0;float bv=p[0];finite=std::isfinite(bv);for(uint32_t i=1;i<n;++i){float x=p[i];if(!std::isfinite(x))finite=false;if(x>bv){bv=x;bi=i;}}return bi;}


static std::string profile_category(const std::string& n){
    if(n=="token_embedding")return "embedding";
    if(n.find("ffn_gate")!=std::string::npos||n.find("ffn_up")!=std::string::npos)return "ffn_gate_up";
    if(n.find("ffn_down")!=std::string::npos)return "ffn_down";
    if(n.find("q_proj")!=std::string::npos||n.find("k_proj")!=std::string::npos||n.find("v_proj")!=std::string::npos)return "attn_qkv";
    if(n.find("o_proj")!=std::string::npos)return "attn_output";
    if(n.find("rmsnorm")!=std::string::npos||n=="output_norm")return "rmsnorm";
    if(n.find("rope")!=std::string::npos)return "rope";
    if(n.find("kv_store")!=std::string::npos)return "kv_store";
    if(n.find("gqa")!=std::string::npos)return "attention";
    if(n.find("residual")!=std::string::npos)return "residual_add";
    if(n.find("swiglu")!=std::string::npos)return "swiglu";
    if(n=="lm_head")return "lm_head";
    return "other";
}
struct CatTick {std::string name;uint64_t ticks=0;uint32_t dispatches=0;};
static std::vector<CatTick> aggregate_profile(const std::vector<DispatchOp>& ops,const ProfileStats& ps){
    std::map<std::string,CatTick> m;for(size_t i=0;i<ops.size();++i){std::string c=profile_category(ops[i].name);auto& x=m[c];x.name=c;x.ticks+=ps.op_ticks[i];x.dispatches++;}std::vector<CatTick> v;for(auto& kv:m)v.push_back(kv.second);std::sort(v.begin(),v.end(),[](const CatTick&a,const CatTick&b){return a.ticks>b.ticks;});return v;
}
static std::vector<std::pair<std::string,uint64_t>> top_ops(const std::vector<DispatchOp>& ops,const ProfileStats& ps,size_t n){
    std::vector<std::pair<std::string,uint64_t>> v;for(size_t i=0;i<ops.size();++i)v.push_back({ops[i].name,ps.op_ticks[i]});std::sort(v.begin(),v.end(),[](const auto&a,const auto&b){return a.second>b.second;});if(v.size()>n)v.resize(n);return v;
}
static void write_profile_json(std::ofstream& o,const std::vector<DispatchOp>& ops,const ProfileStats& ps){
    auto cats=aggregate_profile(ops,ps);auto top=top_ops(ops,ps,12);o<<"{\"timestamp_valid_bits\":"<<ps.timestamp_valid_bits<<",\"chain_ticks\":"<<ps.chain_ticks<<",\"dispatch_tick_sum\":"<<ps.dispatch_tick_sum<<",\"barrier_or_unattributed_ticks\":"<<ps.barrier_or_unattributed_ticks<<",\"wall_ms\":"<<ps.chain.record_submit_wait_ms<<",\"submit_wait_ms\":"<<ps.chain.submit_wait_ms<<",\"categories\":[";for(size_t i=0;i<cats.size();++i){if(i)o<<",";double sh=ps.chain_ticks?double(cats[i].ticks)/double(ps.chain_ticks):0.0;o<<"{\"name\":\""<<cats[i].name<<"\",\"ticks\":"<<cats[i].ticks<<",\"dispatches\":"<<cats[i].dispatches<<",\"chain_share\":"<<sh<<"}";}o<<"],\"top_ops\":[";for(size_t i=0;i<top.size();++i){if(i)o<<",";double sh=ps.chain_ticks?double(top[i].second)/double(ps.chain_ticks):0.0;o<<"{\"name\":\""<<top[i].first<<"\",\"ticks\":"<<top[i].second<<",\"chain_share\":"<<sh<<"}";}o<<"]}";
}

int main(int argc,char** argv){
 std::string model,shader_dir,out_path="p7a_baseline_results.json";try{
  for(int i=1;i<argc;++i){std::string a=argv[i];auto need=[&](const char* f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};if(a=="--model")model=need("--model");else if(a=="--shader-dir")shader_dir=need("--shader-dir");else if(a=="--out")out_path=need("--out");}
  if(model.empty()||shader_dir.empty())throw std::runtime_error("--model and --shader-dir are required");
  GgufReader reader(model);GgufInfo gguf=reader.read();TensorStore store;TensorStoreReport ts=store.inspect(model,gguf);if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)throw std::runtime_error("P1 tensor-store invariants failed");
  const uint32_t hidden=1536,q_heads=12,kv_heads=2,head_dim=128,kv_dim=256,ffn=8960,layers=28,vocab=151936,Q4_K=12,Q6_K=14,F32=0;const uint32_t pp=512,tg=128,max_ctx=640,pos_base=0;
  if(gguf.tensors.size()!=338u)throw std::runtime_error("P7B expects frozen 338-tensor model");auto bc=gguf.scalars.find("qwen2.block_count");if(bc==gguf.scalars.end()||std::stoul(bc->second)!=layers)throw std::runtime_error("block_count mismatch");
  const auto* emb=find_tensor(gguf,"token_embd.weight");const auto* out_norm=find_tensor(gguf,"output_norm.weight");require_dims(emb,Q6_K,hidden,vocab,"token_embd.weight");require_vec(out_norm,F32,hidden,"output_norm.weight");
  struct LRef{const GgufTensorInfo *an,*qw,*kw,*vw,*qb,*kb,*vb,*ow,*fn,*gw,*uw,*dw;};std::vector<LRef> lr(layers);auto nm=[](uint32_t l,const char* s){return std::string("blk.")+std::to_string(l)+s;};
  for(uint32_t l=0;l<layers;++l){auto A=nm(l,".attn_norm.weight"),QW=nm(l,".attn_q.weight"),KW=nm(l,".attn_k.weight"),VW=nm(l,".attn_v.weight"),QB=nm(l,".attn_q.bias"),KB=nm(l,".attn_k.bias"),VB=nm(l,".attn_v.bias"),OW=nm(l,".attn_output.weight"),FN=nm(l,".ffn_norm.weight"),GW=nm(l,".ffn_gate.weight"),UW=nm(l,".ffn_up.weight"),DW=nm(l,".ffn_down.weight");lr[l]={find_tensor(gguf,A),find_tensor(gguf,QW),find_tensor(gguf,KW),find_tensor(gguf,VW),find_tensor(gguf,QB),find_tensor(gguf,KB),find_tensor(gguf,VB),find_tensor(gguf,OW),find_tensor(gguf,FN),find_tensor(gguf,GW),find_tensor(gguf,UW),find_tensor(gguf,DW)};require_vec(lr[l].an,F32,hidden,A.c_str());require_dims(lr[l].qw,Q4_K,hidden,hidden,QW.c_str());require_dims(lr[l].kw,Q4_K,hidden,kv_dim,KW.c_str());require_quant_dims(lr[l].vw,hidden,kv_dim,VW.c_str());require_vec(lr[l].qb,F32,hidden,QB.c_str());require_vec(lr[l].kb,F32,kv_dim,KB.c_str());require_vec(lr[l].vb,F32,kv_dim,VB.c_str());require_dims(lr[l].ow,Q4_K,hidden,hidden,OW.c_str());require_vec(lr[l].fn,F32,hidden,FN.c_str());require_dims(lr[l].gw,Q4_K,hidden,ffn,GW.c_str());require_dims(lr[l].uw,Q4_K,hidden,ffn,UW.c_str());require_quant_dims(lr[l].dw,ffn,hidden,DW.c_str());}
  float eps=1e-6f;auto ei=gguf.scalars.find("qwen2.attention.layer_norm_rms_epsilon");if(ei!=gguf.scalars.end())try{eps=std::stof(ei->second);}catch(...){}float theta=1000000.0f;auto ri=gguf.scalars.find("qwen2.rope.freq_base");if(ri!=gguf.scalars.end())try{theta=std::stof(ri->second);}catch(...){}
  std::cout<<"ArcLLM P7-H post-FFN-tile16 re-profile\n";std::cout<<"layers="<<layers<<" hidden="<<hidden<<" pp="<<pp<<" tg="<<tg<<" max_ctx="<<max_ctx<<"\n";
  VkRuntime vk;vk.init();auto tres0=std::chrono::steady_clock::now();std::vector<ArenaPlan> plans=build_tensor_aware_arenas(gguf,ts.tensor_bytes_total);if(plans.size()!=4u)throw std::runtime_error("P7B frozen arena plan expected 4 arenas");std::vector<Buffer> arenas;for(const auto&a:plans)arenas.push_back(vk.make_buffer(a.end-a.start,store.mapped_base()+gguf.data_offset+a.start));double residency_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tres0).count();
  std::map<std::string,TensorLoc> loc;for(const auto&t:gguf.tensors){uint64_t nb=tensor_nbytes(t);bool found=false;for(uint32_t ai=0;ai<uint32_t(plans.size());++ai)if(t.offset>=plans[ai].start&&t.offset+nb<=plans[ai].end){uint64_t base=t.offset-plans[ai].start;if(base>0xffffffffull)throw std::runtime_error("arena base exceeds uint32");loc[t.name]={ai,uint32_t(base),nb};found=true;break;}if(!found)throw std::runtime_error("tensor crosses P7B arena: "+t.name);}if(loc.size()!=338u)throw std::runtime_error("resident tensor count mismatch");auto L=[&](const std::string&n){auto it=loc.find(n);if(it==loc.end())throw std::runtime_error("missing resident tensor: "+n);return it->second;};auto AB=[&](const std::string&n)->Buffer*{auto z=L(n);return &arenas[z.arena];};auto BASE=[&](const std::string&n){return L(n).byte_base;};auto FBASE=[&](const std::string&n){uint32_t b=BASE(n);if(b%4u)throw std::runtime_error("unaligned F32 tensor");return b/4u;};
  float zero=0.0f;auto fbuf=[&](uint64_t n){return vk.make_buffer(n*sizeof(float));};struct PCEmb{uint32_t n,batch,row_bytes,w_base_bytes;};struct PCRms{uint32_t n,batch;float eps;uint32_t w_base;};struct PCGemm{uint32_t n,rows,batch,row_bytes,add_bias,w_base_bytes,bias_base;};struct PCRope{uint32_t batch,heads,head_dim,pos_base;float theta;};struct PCAttnPrefill{uint32_t q_heads,kv_heads,seq,dim;float scale;};struct PCKV{uint32_t layer,cache_pos_start,batch,max_ctx,kv_dim;};struct PCAttnKV{uint32_t layer,pos,max_ctx,q_heads,kv_heads,dim;float scale;};struct PCN{uint32_t n;};struct PCLM{uint32_t n,row_bytes,row_start,row_count,w_base_bytes,x_base;};
  auto addop=[&](std::vector<DispatchOp>&ops,const std::string&name,const std::string&sh,std::vector<Buffer*> bufs,std::vector<uint8_t>push,uint32_t gx,uint32_t gy=1,uint32_t gz=1){DispatchOp o;o.name=name;o.spv_path=shader_dir+"\\"+sh;o.buffers=std::move(bufs);o.push=std::move(push);o.gx=gx;o.gy=gy;o.gz=gz;ops.push_back(std::move(o));};
  TensorLoc el=L("token_embd.weight");uint32_t qrb=q4_row_bytes(hidden),krb=q4_row_bytes(hidden),orb=q4_row_bytes(hidden),gurb=q4_row_bytes(hidden);const uint32_t lm_chunk=8192,lm_dispatches=(vocab+lm_chunk-1u)/lm_chunk;

  // Regression 1: 2-D Q4/Q6 packed GEMM on real layer-0 weights.
  std::vector<float> rx(uint64_t(4)*hidden);for(size_t i=0;i<rx.size();++i)rx[i]=0.2f*std::sin(float(i+1)*0.013f);Buffer brx=vk.make_buffer(rx.size()*sizeof(float),rx.data()),bry4=fbuf(uint64_t(4)*hidden),bry6=fbuf(uint64_t(4)*kv_dim),bdummy=vk.make_buffer(sizeof(float),&zero);auto QW0=nm(0,".attn_q.weight"),QB0=nm(0,".attn_q.bias"),VW0=nm(0,".attn_v.weight"),VB0=nm(0,".attn_v.bias");const float* qb0=reinterpret_cast<const float*>(tensor_ptr(store,gguf,lr[0].qb));const float* vb0=reinterpret_cast<const float*>(tensor_ptr(store,gguf,lr[0].vb));auto refq=matmul_q4_cpu(tensor_ptr(store,gguf,lr[0].qw),hidden,hidden,4,rx,qb0);std::vector<float> refv=(lr[0].vw->ggml_type==Q4_K)?matmul_q4_cpu(tensor_ptr(store,gguf,lr[0].vw),hidden,kv_dim,4,rx,vb0):matmul_q6_cpu(tensor_ptr(store,gguf,lr[0].vw),hidden,kv_dim,4,rx,vb0);std::vector<DispatchOp> rops;PCGemm rgq{hidden,hidden,4,qrb,1,BASE(QW0),FBASE(QB0)};addop(rops,"reg_q4","p7_q4k_gemm_2d.spv",{AB(QW0),&brx,AB(QB0),&bry4},push_bytes(rgq),(hidden+63u)/64u,4);uint32_t vrb0=(lr[0].vw->ggml_type==Q4_K)?q4_row_bytes(hidden):q6_row_bytes(hidden);PCGemm rgv{hidden,kv_dim,4,vrb0,1,BASE(VW0),FBASE(VB0)};addop(rops,"reg_q6",lr[0].vw->ggml_type==Q4_K?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv",{AB(VW0),&brx,AB(VB0),&bry6},push_bytes(rgv),(kv_dim+63u)/64u,4);PreparedChain rprep=vk.prepare_chain(rops);auto rst=vk.execute_prepared(rprep,rops,false);std::vector<float> gy4(refq.size()),gy6(refv.size());std::memcpy(gy4.data(),bry4.mapped,gy4.size()*sizeof(float));std::memcpy(gy6.data(),bry6.mapped,gy6.size()*sizeof(float));Metrics mq=compare_vec(refq,gy4,0.02,0.005),mvq=compare_vec(refv,gy6,0.02,0.005);vk.destroy_prepared(rprep);
  // Regression 2: scalable online attention at seq=4 and cached pos=4.
  const uint32_t rs=4,rmc=16;std::vector<float> rq(uint64_t(rs)*hidden),rk(uint64_t(rs)*kv_dim),rv(uint64_t(rs)*kv_dim);for(size_t i=0;i<rq.size();++i)rq[i]=0.1f*std::cos(float(i+3)*0.007f);for(size_t i=0;i<rk.size();++i)rk[i]=0.1f*std::sin(float(i+5)*0.011f);for(size_t i=0;i<rv.size();++i)rv[i]=0.08f*std::cos(float(i+7)*0.017f);auto refa=attention_cpu(rq,rk,rv,rs,q_heads,kv_heads,head_dim);Buffer brq=vk.make_buffer(rq.size()*sizeof(float),rq.data()),brk=vk.make_buffer(rk.size()*sizeof(float),rk.data()),brv=vk.make_buffer(rv.size()*sizeof(float),rv.data()),bra=fbuf(refa.size());PCAttnPrefill rap{q_heads,kv_heads,rs,head_dim,1.0f/std::sqrt(float(head_dim))};std::vector<DispatchOp> aops;addop(aops,"reg_attn_prefill","p7_attention_prefill_online.spv",{&brq,&brk,&brv,&bra},push_bytes(rap),rs*q_heads);PreparedChain ap=vk.prepare_chain(aops);vk.execute_prepared(ap,aops,false);std::vector<float> ga(refa.size());std::memcpy(ga.data(),bra.mapped,ga.size()*sizeof(float));Metrics ma=compare_vec(refa,ga,0.0002,0.00005);vk.destroy_prepared(ap);
  std::vector<float> ck(uint64_t(rmc)*kv_dim,0.0f),cv(uint64_t(rmc)*kv_dim,0.0f),cq(hidden);for(uint32_t t=0;t<4;++t){std::copy(rk.begin()+uint64_t(t)*kv_dim,rk.begin()+uint64_t(t+1)*kv_dim,ck.begin()+uint64_t(t)*kv_dim);std::copy(rv.begin()+uint64_t(t)*kv_dim,rv.begin()+uint64_t(t+1)*kv_dim,cv.begin()+uint64_t(t)*kv_dim);}for(size_t i=0;i<cq.size();++i)cq[i]=0.1f*std::sin(float(i+9)*0.009f);for(uint32_t d=0;d<kv_dim;++d){ck[uint64_t(4)*kv_dim+d]=0.07f*std::cos(float(d+1)*0.02f);cv[uint64_t(4)*kv_dim+d]=0.05f*std::sin(float(d+1)*0.03f);}std::vector<float> cka(uint64_t(layers)*rmc*kv_dim,0.0f),cva(cka.size(),0.0f);std::copy(ck.begin(),ck.end(),cka.begin());std::copy(cv.begin(),cv.end(),cva.begin());auto refc=attention_cache_cpu(cq,cka,cva,0,4,rmc,q_heads,kv_heads,head_dim);Buffer bcq=vk.make_buffer(cq.size()*sizeof(float),cq.data()),bck=vk.make_buffer(cka.size()*sizeof(float),cka.data()),bcv=vk.make_buffer(cva.size()*sizeof(float),cva.data()),bca=fbuf(refc.size());PCAttnKV rak{0,4,rmc,q_heads,kv_heads,head_dim,1.0f/std::sqrt(float(head_dim))};std::vector<DispatchOp> cops;addop(cops,"reg_attn_kv","p7_attention_kv_online.spv",{&bcq,&bck,&bcv,&bca},push_bytes(rak),q_heads);PreparedChain cp=vk.prepare_chain(cops);vk.execute_prepared(cp,cops,false);std::vector<float> gc(refc.size());std::memcpy(gc.data(),bca.mapped,gc.size()*sizeof(float));Metrics mc=compare_vec(refc,gc,0.0002,0.00005);vk.destroy_prepared(cp);bool regression_pass=mq.pass&&mvq.pass&&ma.pass&&mc.pass;std::cout<<"kernel regression q4="<<mq.pass<<" q6/mixed="<<mvq.pass<<" prefill_attn="<<ma.pass<<" kv_attn="<<mc.pass<<"\n";if(!regression_pass)throw std::runtime_error("P7-B inherited changed-kernel regression failed");
  for(Buffer* b:{&bca,&bcv,&bck,&bcq,&bra,&brv,&brk,&brq,&bry6,&bry4,&brx})vk.destroy_buffer(*b);

  // Production buffers sized for pp512/tg128.
  std::vector<uint32_t> prompt(pp);for(uint32_t i=0;i<pp;++i)prompt[i]=uint32_t((17ull+7919ull*i)%vocab);uint32_t decode_id=0;Buffer b_ids=vk.make_buffer(uint64_t(pp)*sizeof(uint32_t),prompt.data()),b_dec_id=vk.make_buffer(sizeof(uint32_t),&decode_id);auto fb=[&](uint64_t n){return vk.make_buffer(n*sizeof(float));};Buffer b_h0=fb(uint64_t(pp)*hidden),b_h1=fb(uint64_t(pp)*hidden),b_n1=fb(uint64_t(pp)*hidden),b_q=fb(uint64_t(pp)*hidden),b_k=fb(uint64_t(pp)*kv_dim),b_v=fb(uint64_t(pp)*kv_dim),b_qr=fb(uint64_t(pp)*hidden),b_kr=fb(uint64_t(pp)*kv_dim),b_attn=fb(uint64_t(pp)*hidden),b_o=fb(uint64_t(pp)*hidden),b_r1=fb(uint64_t(pp)*hidden),b_n2=fb(uint64_t(pp)*hidden),b_g=fb(uint64_t(pp)*ffn),b_u=fb(uint64_t(pp)*ffn),b_s=fb(uint64_t(pp)*ffn),b_d=fb(uint64_t(pp)*hidden),b_norm=fb(uint64_t(pp)*hidden),b_logits=fb(vocab);uint64_t cache_elems=uint64_t(layers)*max_ctx*kv_dim;Buffer b_kcache=fb(cache_elems),b_vcache=fb(cache_elems);
  auto build_prefill=[&](){std::vector<DispatchOp> ops;PCEmb pe{hidden,pp,q6_row_bytes(hidden),el.byte_base};addop(ops,"token_embedding","p7_embedding_q6k.spv",{&arenas[el.arena],&b_ids,&b_h0},push_bytes(pe),(pp*hidden+255u)/256u);Buffer*cur=&b_h0;Buffer*nxt=&b_h1;for(uint32_t l=0;l<layers;++l){std::string pr="L"+(l<10?std::string("0"):std::string())+std::to_string(l)+".";auto AN=nm(l,".attn_norm.weight"),QW=nm(l,".attn_q.weight"),KW=nm(l,".attn_k.weight"),VW=nm(l,".attn_v.weight"),QB=nm(l,".attn_q.bias"),KB=nm(l,".attn_k.bias"),VB=nm(l,".attn_v.bias"),OW=nm(l,".attn_output.weight"),FN=nm(l,".ffn_norm.weight"),GW=nm(l,".ffn_gate.weight"),UW=nm(l,".ffn_up.weight"),DW=nm(l,".ffn_down.weight");PCRms rn{hidden,pp,eps,FBASE(AN)};addop(ops,pr+"attn_rmsnorm","p7_rmsnorm_seq.spv",{cur,AB(AN),&b_n1},push_bytes(rn),pp);uint32_t vrb=(lr[l].vw->ggml_type==Q4_K)?q4_row_bytes(hidden):q6_row_bytes(hidden);PCGemm pq{hidden,hidden,pp,qrb,1,BASE(QW),FBASE(QB)},pk{hidden,kv_dim,pp,krb,1,BASE(KW),FBASE(KB)},pv{hidden,kv_dim,pp,vrb,1,BASE(VW),FBASE(VB)};addop(ops,pr+"q_proj","p7c_ffn_q4k_tiled.spv",{AB(QW),&b_n1,AB(QB),&b_q},push_bytes(pq),(hidden+7u)/8u,(pp+7u)/8u);addop(ops,pr+"k_proj","p7c_ffn_q4k_tiled.spv",{AB(KW),&b_n1,AB(KB),&b_k},push_bytes(pk),(kv_dim+7u)/8u,(pp+7u)/8u);addop(ops,pr+"v_proj",lr[l].vw->ggml_type==Q4_K?"p7c_ffn_q4k_tiled.spv":"p7c_ffn_q6k_tiled.spv",{AB(VW),&b_n1,AB(VB),&b_v},push_bytes(pv),(kv_dim+7u)/8u,(pp+7u)/8u);PCRope qr{pp,q_heads,head_dim,pos_base,theta},kr{pp,kv_heads,head_dim,pos_base,theta};addop(ops,pr+"q_rope","p7_rope_seq.spv",{&b_q,&b_qr},push_bytes(qr),(pp*q_heads*(head_dim/2u)+127u)/128u);addop(ops,pr+"k_rope","p7_rope_seq.spv",{&b_k,&b_kr},push_bytes(kr),(pp*kv_heads*(head_dim/2u)+127u)/128u);PCKV kv{l,0,pp,max_ctx,kv_dim};addop(ops,pr+"kv_store","p7_kv_store.spv",{&b_kr,&b_v,&b_kcache,&b_vcache},push_bytes(kv),(pp*kv_dim+255u)/256u);PCAttnPrefill at{q_heads,kv_heads,pp,head_dim,1.0f/std::sqrt(float(head_dim))};addop(ops,pr+"causal_gqa","p7_attention_prefill_online.spv",{&b_qr,&b_kr,&b_v,&b_attn},push_bytes(at),pp*q_heads);PCGemm po{hidden,hidden,pp,orb,0,BASE(OW),0};addop(ops,pr+"o_proj","p7c_ffn_q4k_tiled.spv",{AB(OW),&b_attn,&bdummy,&b_o},push_bytes(po),(hidden+7u)/8u,(pp+7u)/8u);PCN ph{pp*hidden};addop(ops,pr+"attn_residual","p7_add.spv",{cur,&b_o,&b_r1},push_bytes(ph),(ph.n+255u)/256u);PCRms fn{hidden,pp,eps,FBASE(FN)};addop(ops,pr+"ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b_r1,AB(FN),&b_n2},push_bytes(fn),pp);PCGemm pg{hidden,ffn,pp,gurb,0,BASE(GW),0},pu{hidden,ffn,pp,gurb,0,BASE(UW),0};addop(ops,pr+"ffn_gate","p7g_ffn_q4k_tiled16.spv",{AB(GW),&b_n2,&bdummy,&b_g},push_bytes(pg),(ffn+7u)/8u,(pp+15u)/16u);addop(ops,pr+"ffn_up","p7g_ffn_q4k_tiled16.spv",{AB(UW),&b_n2,&bdummy,&b_u},push_bytes(pu),(ffn+7u)/8u,(pp+15u)/16u);PCN pf{pp*ffn};addop(ops,pr+"swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(pf),(pf.n+255u)/256u);uint32_t drb=lr[l].dw->ggml_type==Q4_K?q4_row_bytes(ffn):q6_row_bytes(ffn);PCGemm pd{ffn,hidden,pp,drb,0,BASE(DW),0};addop(ops,pr+"ffn_down",lr[l].dw->ggml_type==Q4_K?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv",{AB(DW),&b_s,&bdummy,&b_d},push_bytes(pd),(hidden+7u)/8u,(pp+15u)/16u);addop(ops,pr+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(ph),(ph.n+255u)/256u);std::swap(cur,nxt);}auto ON=std::string("output_norm.weight");PCRms on{hidden,pp,eps,FBASE(ON)};addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB(ON),&b_norm},push_bytes(on),pp);for(uint32_t rs0=0;rs0<vocab;rs0+=lm_chunk){uint32_t rc=(std::min)(lm_chunk,vocab-rs0);PCLM lm{hidden,q6_row_bytes(hidden),rs0,rc,el.byte_base,(pp-1u)*hidden};addop(ops,"lm_head","p7_lmhead_q6k_chunk.spv",{&arenas[el.arena],&b_norm,&b_logits},push_bytes(lm),(rc+63u)/64u);}return ops;};
  auto build_decode=[&](uint32_t pos){std::vector<DispatchOp> ops;PCEmb pe{hidden,1,q6_row_bytes(hidden),el.byte_base};addop(ops,"token_embedding","p7_embedding_q6k.spv",{&arenas[el.arena],&b_dec_id,&b_h0},push_bytes(pe),(hidden+255u)/256u);Buffer*cur=&b_h0;Buffer*nxt=&b_h1;for(uint32_t l=0;l<layers;++l){std::string pr="L"+(l<10?std::string("0"):std::string())+std::to_string(l)+".";auto AN=nm(l,".attn_norm.weight"),QW=nm(l,".attn_q.weight"),KW=nm(l,".attn_k.weight"),VW=nm(l,".attn_v.weight"),QB=nm(l,".attn_q.bias"),KB=nm(l,".attn_k.bias"),VB=nm(l,".attn_v.bias"),OW=nm(l,".attn_output.weight"),FN=nm(l,".ffn_norm.weight"),GW=nm(l,".ffn_gate.weight"),UW=nm(l,".ffn_up.weight"),DW=nm(l,".ffn_down.weight");PCRms rn{hidden,1,eps,FBASE(AN)};addop(ops,pr+"attn_rmsnorm","p7_rmsnorm_seq.spv",{cur,AB(AN),&b_n1},push_bytes(rn),1);uint32_t vrb=lr[l].vw->ggml_type==Q4_K?q4_row_bytes(hidden):q6_row_bytes(hidden);PCGemm pq{hidden,hidden,1,qrb,1,BASE(QW),FBASE(QB)},pk{hidden,kv_dim,1,krb,1,BASE(KW),FBASE(KB)},pv{hidden,kv_dim,1,vrb,1,BASE(VW),FBASE(VB)};addop(ops,pr+"q_proj","p7_q4k_gemm_2d.spv",{AB(QW),&b_n1,AB(QB),&b_q},push_bytes(pq),(hidden+63u)/64u,1);addop(ops,pr+"k_proj","p7_q4k_gemm_2d.spv",{AB(KW),&b_n1,AB(KB),&b_k},push_bytes(pk),(kv_dim+63u)/64u,1);addop(ops,pr+"v_proj",lr[l].vw->ggml_type==Q4_K?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv",{AB(VW),&b_n1,AB(VB),&b_v},push_bytes(pv),(kv_dim+63u)/64u,1);PCRope qr{1,q_heads,head_dim,pos,theta},kr{1,kv_heads,head_dim,pos,theta};addop(ops,pr+"q_rope","p7_rope_seq.spv",{&b_q,&b_qr},push_bytes(qr),(q_heads*(head_dim/2u)+127u)/128u);addop(ops,pr+"k_rope","p7_rope_seq.spv",{&b_k,&b_kr},push_bytes(kr),(kv_heads*(head_dim/2u)+127u)/128u);PCKV kv{l,pos,1,max_ctx,kv_dim};addop(ops,pr+"kv_store","p7_kv_store.spv",{&b_kr,&b_v,&b_kcache,&b_vcache},push_bytes(kv),(kv_dim+255u)/256u);PCAttnKV at{l,pos,max_ctx,q_heads,kv_heads,head_dim,1.0f/std::sqrt(float(head_dim))};addop(ops,pr+"cached_gqa","p7_attention_kv_online.spv",{&b_qr,&b_kcache,&b_vcache,&b_attn},push_bytes(at),q_heads);PCGemm po{hidden,hidden,1,orb,0,BASE(OW),0};addop(ops,pr+"o_proj","p7_q4k_gemm_2d.spv",{AB(OW),&b_attn,&bdummy,&b_o},push_bytes(po),(hidden+63u)/64u,1);PCN ph{hidden};addop(ops,pr+"attn_residual","p7_add.spv",{cur,&b_o,&b_r1},push_bytes(ph),(hidden+255u)/256u);PCRms fn{hidden,1,eps,FBASE(FN)};addop(ops,pr+"ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b_r1,AB(FN),&b_n2},push_bytes(fn),1);PCGemm pg{hidden,ffn,1,gurb,0,BASE(GW),0},pu{hidden,ffn,1,gurb,0,BASE(UW),0};addop(ops,pr+"ffn_gate","p7_q4k_gemm_2d.spv",{AB(GW),&b_n2,&bdummy,&b_g},push_bytes(pg),(ffn+63u)/64u,1);addop(ops,pr+"ffn_up","p7_q4k_gemm_2d.spv",{AB(UW),&b_n2,&bdummy,&b_u},push_bytes(pu),(ffn+63u)/64u,1);PCN pf{ffn};addop(ops,pr+"swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(pf),(ffn+255u)/256u);uint32_t drb=lr[l].dw->ggml_type==Q4_K?q4_row_bytes(ffn):q6_row_bytes(ffn);PCGemm pd{ffn,hidden,1,drb,0,BASE(DW),0};addop(ops,pr+"ffn_down",lr[l].dw->ggml_type==Q4_K?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv",{AB(DW),&b_s,&bdummy,&b_d},push_bytes(pd),(hidden+63u)/64u,1);addop(ops,pr+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(ph),(hidden+255u)/256u);std::swap(cur,nxt);}auto ON=std::string("output_norm.weight");PCRms on{hidden,1,eps,FBASE(ON)};addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB(ON),&b_norm},push_bytes(on),1);for(uint32_t rs0=0;rs0<vocab;rs0+=lm_chunk){uint32_t rc=(std::min)(lm_chunk,vocab-rs0);PCLM lm{hidden,q6_row_bytes(hidden),rs0,rc,el.byte_base,0};addop(ops,"lm_head","p7_lmhead_q6k_chunk.spv",{&arenas[el.arena],&b_norm,&b_logits},push_bytes(lm),(rc+63u)/64u);}return ops;};
  auto ppops=build_prefill();auto dtemplate=build_decode(pp);const uint32_t expected=1u+layers*16u+1u+lm_dispatches;if(ppops.size()!=expected||dtemplate.size()!=expected)throw std::runtime_error("P7F dispatch plan mismatch");std::cout<<"Preparing P7-E optimized prefill graph for timestamp re-profile: "<<expected<<" dispatches each\n";PreparedChain ppc=vk.prepare_chain(ppops),dc=vk.prepare_chain(dtemplate);std::cout<<"prepare prefill_ms="<<ppc.prepare_ms<<" decode_ms="<<dc.prepare_ms<<" timestamp_valid_bits="<<vk.timestamp_valid_bits()<<"\n";
  ProfileStats ppst=vk.execute_profiled(ppc,ppops,false);bool fin1=true;uint32_t first_decode=argmax_ptr(reinterpret_cast<const float*>(b_logits.mapped),vocab,fin1);std::memcpy(b_dec_id.mapped,&first_decode,sizeof(first_decode));auto dops=build_decode(pp);ProfileStats dst=vk.execute_profiled(dc,dops,true);bool fin2=true;uint32_t decode_top=argmax_ptr(reinterpret_cast<const float*>(b_logits.mapped),vocab,fin2);bool allfinite=fin1&&fin2;
  auto ppcats=aggregate_profile(ppops,ppst),dcats=aggregate_profile(dops,dst);std::cout<<"P7-F optimized prefill profile wall_ms="<<ppst.chain.record_submit_wait_ms<<" submit_wait_ms="<<ppst.chain.submit_wait_ms<<" chain_ticks="<<ppst.chain_ticks<<" top1="<<first_decode<<"\n";for(size_t i=0;i<(std::min)(size_t(5),ppcats.size());++i)std::cout<<"  pp "<<ppcats[i].name<<" share="<<(ppst.chain_ticks?double(ppcats[i].ticks)/double(ppst.chain_ticks):0.0)<<" dispatches="<<ppcats[i].dispatches<<"\n";std::cout<<"P7-F unchanged decode profile wall_ms="<<dst.chain.record_submit_wait_ms<<" submit_wait_ms="<<dst.chain.submit_wait_ms<<" chain_ticks="<<dst.chain_ticks<<" top1="<<decode_top<<"\n";for(size_t i=0;i<(std::min)(size_t(5),dcats.size());++i)std::cout<<"  tg "<<dcats[i].name<<" share="<<(dst.chain_ticks?double(dcats[i].ticks)/double(dst.chain_ticks):0.0)<<" dispatches="<<dcats[i].dispatches<<"\n";
  bool pass=regression_pass&&allfinite&&vk.timestamp_valid_bits()>0&&ppst.chain_ticks>0&&dst.chain_ticks>0&&ppst.dispatch_tick_sum>0&&dst.dispatch_tick_sum>0;std::ofstream o(out_path,std::ios::binary);if(!o)throw std::runtime_error("cannot write result");o<<std::setprecision(10)<<"{\n  \"schema\":\"arcllm.p7h.post_tile16_reprofile.v1\",\n  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";o<<"  \"model\":{\"layers\":28,\"tensors\":338,\"resident_weight_bytes\":980097536,\"pp_tokens\":512,\"profile_decode_pos\":512,\"max_ctx\":640},\n";o<<"  \"regression\":{\"q4_2d_pass\":"<<json_bool(mq.pass)<<",\"q6_or_mixed_2d_pass\":"<<json_bool(mvq.pass)<<",\"prefill_online_pass\":"<<json_bool(ma.pass)<<",\"kv_online_pass\":"<<json_bool(mc.pass)<<",\"pass\":"<<json_bool(regression_pass)<<"},\n";o<<"  \"runtime\":{\"all_weights_resident\":true,\"q4k_direct_packed\":true,\"q6k_direct_packed\":true,\"expanded_before_shader\":false,\"kv_gpu_resident\":true,\"prepared_chain_reuse\":true,\"dispatches_per_pass\":"<<expected<<"},\n";o<<"  \"prefill_profile\":";write_profile_json(o,ppops,ppst);o<<",\n  \"decode_profile\":";write_profile_json(o,dops,dst);o<<",\n";o<<"  \"outputs\":{\"prefill_top1\":"<<first_decode<<",\"decode_top1\":"<<decode_top<<",\"finite\":"<<json_bool(allfinite)<<"},\n";o<<"  \"p7h_gate\":{\"pass\":"<<json_bool(pass)<<",\"performance_threshold_applied\":false,\"p7_closed\":false,\"criteria\":\"P7-G optimized prefill graph with frozen attention-projection tile8 plus FFN token-tile16 and unchanged decode graph profile with supported Vulkan timestamps; inherited regressions remain PASS; finite outputs; nonzero whole-chain and per-dispatch device ticks\"}\n}\n";o.close();
  vk.destroy_prepared(dc);vk.destroy_prepared(ppc);std::vector<Buffer*> scratch={&b_vcache,&b_kcache,&b_logits,&b_norm,&b_d,&b_s,&b_u,&b_g,&b_n2,&b_r1,&b_o,&b_attn,&b_kr,&b_qr,&b_v,&b_k,&b_q,&b_n1,&b_h1,&b_h0,&b_dec_id,&b_ids,&bdummy};for(Buffer*b:scratch)vk.destroy_buffer(*b);for(auto&b:arenas)vk.destroy_buffer(b);return pass?0:20;
 }catch(const std::exception&e){std::cerr<<"P7-H error: "<<e.what()<<"\n";try{std::ofstream o(out_path,std::ios::binary);if(o)o<<"{\n  \"schema\":\"arcllm.p7h.post_tile16_reprofile.v1\",\n  \"status\":\"ERROR\",\n  \"p7h_gate\":{\"pass\":false,\"p7_closed\":false}\n}\n";}catch(...){}return 2;}
}
