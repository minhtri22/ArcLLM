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
using PFN_vkCreateQueryPool = VkResult (WINAPI*)(VkDevice, const VkQueryPoolCreateInfo*, const void*, VkQueryPool*);
using PFN_vkDestroyQueryPool = void (WINAPI*)(VkDevice, VkQueryPool, const void*);
using PFN_vkCmdResetQueryPool = void (WINAPI*)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t);
using PFN_vkCmdWriteTimestamp = void (WINAPI*)(VkCommandBuffer, VkPipelineStageFlags, VkQueryPool, uint32_t);
using PFN_vkGetQueryPoolResults = VkResult (WINAPI*)(VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void*, VkDeviceSize, VkQueryResultFlags);
using PFN_vkGetPhysicalDeviceProperties = void (WINAPI*)(VkPhysicalDevice, VkPhysicalDeviceProperties*);

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
    double timestamp_period_ns = 0.0;
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
        ai.pApplicationName = "ArcLLM-P7A";
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
        auto get_props = req<PFN_vkGetPhysicalDeviceProperties>(gipa_, instance_, "vkGetPhysicalDeviceProperties");
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
        VkPhysicalDeviceProperties phys_props{};
        get_props(phys_, &phys_props);
        timestamp_period_ns_ = double(phys_props.limits.timestampPeriod);

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
    double timestamp_period_ns() const { return timestamp_period_ns_; }

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
        auto tall0=std::chrono::steady_clock::now();VkCommandPoolCreateInfo pci{};pci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pci.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;pci.queueFamilyIndex=queue_family_;VkCommandPool pool=nullptr;VkResult vr=create_command_pool_(device_,&pci,nullptr,&pool);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateCommandPool failed P7A");VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer cb=nullptr;vr=allocate_command_buffers_(device_,&ai,&cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkAllocateCommandBuffers failed P7A");VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;vr=begin_command_buffer_(cb,&bi);if(vr!=VK_SUCCESS)throw std::runtime_error("vkBeginCommandBuffer failed P7A");ChainStats st{};
        if(cross_submit_compute_barrier){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);st.initial_compute_barrier_count=1;}
        for(size_t oi=0;oi<ops.size();++oi){const auto& op=ops[oi];const auto& p=chain.prepared[oi];cmd_bind_pipeline_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline);cmd_bind_descriptor_sets_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline_layout,0,1,&p.descriptor_set,0,nullptr);if(!op.push.empty())cmd_push_constants_(cb,p.pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,uint32_t(op.push.size()),op.push.data());cmd_dispatch_(cb,op.gx,op.gy,op.gz);++st.dispatch_count;if(oi+1<ops.size()){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);++st.internal_barrier_count;}}
        VkMemoryBarrier hb{};hb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;hb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;hb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&hb,0,nullptr,0,nullptr);st.final_host_barrier_count=1;vr=end_command_buffer_(cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkEndCommandBuffer failed P7A");VkFenceCreateInfo fci{};fci.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;VkFence fence=nullptr;vr=create_fence_(device_,&fci,nullptr,&fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateFence failed P7A");VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;si.commandBufferCount=1;si.pCommandBuffers=&cb;auto ts0=std::chrono::steady_clock::now();vr=queue_submit_(queue_,1,&si,fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkQueueSubmit failed P7A");st.submit_count=1;vr=wait_for_fences_(device_,1,&fence,VK_TRUE,(std::numeric_limits<uint64_t>::max)());if(vr!=VK_SUCCESS)throw std::runtime_error("vkWaitForFences failed P7A");st.fence_wait_count=1;auto ts1=std::chrono::steady_clock::now();st.submit_wait_ms=std::chrono::duration<double,std::milli>(ts1-ts0).count();destroy_fence_(device_,fence,nullptr);destroy_command_pool_(device_,pool,nullptr);st.record_submit_wait_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tall0).count();return st;
    }


    ProfileStats execute_profiled(const PreparedChain& chain,const std::vector<DispatchOp>& ops,bool cross_submit_compute_barrier=false){
        if(timestamp_valid_bits_==0)throw std::runtime_error("I001R compute queue has timestampValidBits=0");
        if(ops.size()!=chain.prepared.size()||ops.empty())throw std::runtime_error("I001R profiled prepared chain shape mismatch");
        for(size_t i=0;i<ops.size();++i){
            const auto& p=chain.prepared[i];
            if(ops[i].spv_path!=p.spv_path||ops[i].push.size()!=p.push_size||ops[i].buffers.size()!=p.buffers.size())
                throw std::runtime_error("I001R profiled op signature mismatch: "+ops[i].name);
            for(size_t j=0;j<ops[i].buffers.size();++j)
                if(ops[i].buffers[j]!=p.buffers[j])throw std::runtime_error("I001R profiled op buffer mismatch: "+ops[i].name);
        }
        auto tick_delta=[&](uint64_t a,uint64_t b)->uint64_t{
            if(timestamp_valid_bits_>=64u)return b-a;
            uint64_t mask=(uint64_t(1)<<timestamp_valid_bits_)-1u;
            return (b-a)&mask;
        };
        ProfileStats ps{};
        ps.timestamp_valid_bits=timestamp_valid_bits_;
        ps.timestamp_period_ns=timestamp_period_ns_;
        ps.op_ticks.resize(ops.size());
        auto tall0=std::chrono::steady_clock::now();

        VkCommandPoolCreateInfo pci{};pci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pci.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;pci.queueFamilyIndex=queue_family_;
        VkCommandPool pool=nullptr;VkResult vr=create_command_pool_(device_,&pci,nullptr,&pool);
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkCreateCommandPool failed");
        VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
        VkCommandBuffer cb=nullptr;vr=allocate_command_buffers_(device_,&ai,&cb);
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkAllocateCommandBuffers failed");

        const uint32_t qcount=uint32_t(ops.size()*2u+2u);
        VkQueryPoolCreateInfo qci{};qci.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        qci.queryType=VK_QUERY_TYPE_TIMESTAMP;qci.queryCount=qcount;
        VkQueryPool qp=nullptr;vr=create_query_pool_(device_,&qci,nullptr,&qp);
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkCreateQueryPool failed");

        VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vr=begin_command_buffer_(cb,&bi);
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkBeginCommandBuffer failed");
        cmd_reset_query_pool_(cb,qp,0,qcount);
        cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,qp,0);

        if(cross_submit_compute_barrier){
            VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;
            mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
            cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);
            ps.chain.initial_compute_barrier_count=1;
        }

        for(size_t oi=0;oi<ops.size();++oi){
            const auto& op=ops[oi];const auto& p=chain.prepared[oi];
            const uint32_t qb=1u+uint32_t(oi)*2u;
            cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,qp,qb);
            cmd_bind_pipeline_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline);
            cmd_bind_descriptor_sets_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline_layout,0,1,&p.descriptor_set,0,nullptr);
            if(!op.push.empty())cmd_push_constants_(cb,p.pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,uint32_t(op.push.size()),op.push.data());
            cmd_dispatch_(cb,op.gx,op.gy,op.gz);
            cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,qp,qb+1u);
            ++ps.chain.dispatch_count;
            if(oi+1<ops.size()){
                VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;
                mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
                cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);
                ++ps.chain.internal_barrier_count;
            }
        }

        VkMemoryBarrier hb{};hb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        hb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;hb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&hb,0,nullptr,0,nullptr);
        ps.chain.final_host_barrier_count=1;
        cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,qp,qcount-1u);
        vr=end_command_buffer_(cb);
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkEndCommandBuffer failed");

        VkFenceCreateInfo fci{};fci.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence=nullptr;vr=create_fence_(device_,&fci,nullptr,&fence);
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkCreateFence failed");
        VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount=1;si.pCommandBuffers=&cb;
        auto ts0=std::chrono::steady_clock::now();
        vr=queue_submit_(queue_,1,&si,fence);
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkQueueSubmit failed");
        ps.chain.submit_count=1;
        vr=wait_for_fences_(device_,1,&fence,VK_TRUE,(std::numeric_limits<uint64_t>::max)());
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkWaitForFences failed");
        ps.chain.fence_wait_count=1;
        auto ts1=std::chrono::steady_clock::now();
        ps.chain.submit_wait_ms=std::chrono::duration<double,std::milli>(ts1-ts0).count();

        std::vector<uint64_t> ticks(qcount);
        vr=get_query_pool_results_(device_,qp,0,qcount,ticks.size()*sizeof(uint64_t),ticks.data(),sizeof(uint64_t),
                                   VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT);
        if(vr!=VK_SUCCESS)throw std::runtime_error("I001R vkGetQueryPoolResults failed");
        ps.chain_ticks=tick_delta(ticks[0],ticks[qcount-1u]);
        for(size_t oi=0;oi<ops.size();++oi){
            const uint32_t qb=1u+uint32_t(oi)*2u;
            const uint64_t d=tick_delta(ticks[qb],ticks[qb+1u]);
            ps.op_ticks[oi]=d;ps.dispatch_tick_sum+=d;
        }
        ps.barrier_or_unattributed_ticks=(ps.chain_ticks>ps.dispatch_tick_sum)?(ps.chain_ticks-ps.dispatch_tick_sum):0;

        destroy_fence_(device_,fence,nullptr);
        destroy_query_pool_(device_,qp,nullptr);
        destroy_command_pool_(device_,pool,nullptr);
        ps.chain.record_submit_wait_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tall0).count();
        return ps;
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
    double timestamp_period_ns_ = 0.0;
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


static void q4k_dequant_row_p8c(const uint8_t* row,float* y,uint32_t n){
    uint32_t nb=n/256u;
    for(uint32_t ib=0;ib<nb;++ib){
        const uint8_t*b=row+uint64_t(ib)*144u;
        uint16_t hd=uint16_t(b[0])|(uint16_t(b[1])<<8);
        uint16_t hm=uint16_t(b[2])|(uint16_t(b[3])<<8);
        float d=half_to_float(hd),dmin=half_to_float(hm);
        for(uint32_t k=0;k<256u;++k){
            uint8_t sc=0,mn=0;scale_min_cpu(b,int(k>>5u),sc,mn);
            uint32_t g=k>>6u,l=k&63u;
            uint8_t qb=b[16u+g*32u+(l&31u)];
            uint8_t q=l<32u?(qb&15u):(qb>>4u);
            y[ib*256u+k]=d*float(sc)*float(q)-dmin*float(mn);
        }
    }
}
static std::string join_path_p8c(const std::string&a,const std::string&b){
    if(a.empty())return b;char c=a.back();return a+((c=='\\'||c=='/')?"":"\\")+b;
}

int main(int argc,char**argv){
    std::string model,shader_dir,out="p8c_access_results.json";
    try{
        for(int i=1;i<argc;++i){
            std::string a=argv[i];
            auto need=[&](const char*f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};
            if(a=="--model")model=need("--model");
            else if(a=="--shader-dir")shader_dir=need("--shader-dir");
            else if(a=="--out")out=need("--out");
        }
        if(model.empty()||shader_dir.empty())throw std::runtime_error("--model and --shader-dir are required");
        GgufInfo gguf=GgufReader(model).read();
        TensorStore store;TensorStoreReport ts=store.inspect(model,gguf);
        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)
            throw std::runtime_error("TensorStore invariants failed");
        const uint32_t hidden=3584,vocab=152064,Q4_K=12,Q6_K=14;
        const uint32_t emb_boundary=133152,out_boundary=91304;
        const uint32_t emb_rb=2016,out_rb=2940;
        const uint64_t emb0_bytes=268434432ull,emb1_bytes=38126592ull;
        const uint64_t out0_start=4230051840ull,out0_bytes=268433760ull,out1_start=4498485600ull,out1_bytes=178634400ull;
        const auto*emb=find_tensor(gguf,"token_embd.weight");
        const auto*outw=find_tensor(gguf,"output.weight");
        require_dims(emb,Q4_K,hidden,vocab,"token_embd.weight");
        require_dims(outw,Q6_K,hidden,vocab,"output.weight");
        if(emb->offset!=0||tensor_nbytes(*emb)!=306561024ull||outw->offset!=out0_start||tensor_nbytes(*outw)!=447068160ull)
            throw std::runtime_error("P8-C frozen segmented tensor geometry mismatch");
        const uint8_t*payload=store.mapped_base()+gguf.data_offset;
        const uint8_t*emb_ptr=tensor_ptr(store,gguf,emb);
        const uint8_t*out_ptr=tensor_ptr(store,gguf,outw);

        std::vector<uint32_t> emb_ids={0u,1u,133150u,133151u,133152u,133153u,152062u,152063u};
        std::vector<uint32_t> lm_rows={0u,1u,91302u,91303u,91304u,91305u,152062u,152063u};

        auto mapping_equivalent=[](uint64_t tensor_offset,uint32_t row_bytes,uint32_t boundary,
                                   uint64_t segment0_source_offset,uint64_t segment1_source_offset,
                                   uint32_t global_row){
            uint64_t local_row=(global_row<boundary)?uint64_t(global_row):uint64_t(global_row-boundary);
            uint64_t segment_source=(global_row<boundary)?segment0_source_offset:segment1_source_offset;
            return segment_source+local_row*uint64_t(row_bytes)==
                   tensor_offset+uint64_t(global_row)*uint64_t(row_bytes);
        };
        bool emb_mapping_equivalence_pass=true;
        for(uint32_t row:emb_ids)
            emb_mapping_equivalence_pass=emb_mapping_equivalence_pass&&
                mapping_equivalent(emb->offset,emb_rb,emb_boundary,emb->offset,emb->offset+emb0_bytes,row);
        bool lm_mapping_equivalence_pass=true;
        for(uint32_t row:lm_rows)
            lm_mapping_equivalence_pass=lm_mapping_equivalence_pass&&
                mapping_equivalent(outw->offset,out_rb,out_boundary,outw->offset,outw->offset+out0_bytes,row);
        bool mapping_equivalence_pass=emb_mapping_equivalence_pass&&lm_mapping_equivalence_pass;
        if(!mapping_equivalence_pass)
            throw std::runtime_error("P8-C pre-dispatch mapping equivalence failed");

        std::vector<float> emb_ref(uint64_t(emb_ids.size())*hidden);
        for(size_t i=0;i<emb_ids.size();++i)
            q4k_dequant_row_p8c(emb_ptr+uint64_t(emb_ids[i])*emb_rb,emb_ref.data()+uint64_t(i)*hidden,hidden);
        std::vector<float>x(hidden);
        for(uint32_t i=0;i<hidden;++i)x[i]=0.5f*std::sin(float(i)*0.013f)+0.25f*std::cos(float(i)*0.007f);
        std::vector<float>lm_ref(lm_rows.size());
        for(size_t i=0;i<lm_rows.size();++i)lm_ref[i]=q6k_dot_row(out_ptr+uint64_t(lm_rows[i])*out_rb,x.data(),hidden);

        VkRuntime vk;vk.init();
        Buffer b_emb0=vk.make_buffer(emb0_bytes,payload);
        Buffer b_emb1=vk.make_buffer(emb1_bytes,payload+emb0_bytes);
        Buffer b_out0=vk.make_buffer(out0_bytes,payload+out0_start);
        Buffer b_out1=vk.make_buffer(out1_bytes,payload+out1_start);
        Buffer b_emb_ids=vk.make_buffer(emb_ids.size()*sizeof(uint32_t),emb_ids.data());
        Buffer b_lm_rows=vk.make_buffer(lm_rows.size()*sizeof(uint32_t),lm_rows.data());
        Buffer b_x=vk.make_buffer(x.size()*sizeof(float),x.data());
        Buffer b_emb_y=vk.make_buffer(emb_ref.size()*sizeof(float));
        Buffer b_lm_y=vk.make_buffer(lm_ref.size()*sizeof(float));
        struct PC{uint32_t n,count,row_bytes,boundary;};
        PC pe{hidden,uint32_t(emb_ids.size()),emb_rb,emb_boundary};
        PC pl{hidden,uint32_t(lm_rows.size()),out_rb,out_boundary};
        std::vector<DispatchOp> ops;
        ops.push_back({"segmented_embedding",join_path_p8c(shader_dir,"p8c_embedding_q4k_segmented_probe.spv"),
                       {&b_emb0,&b_emb1,&b_emb_ids,&b_emb_y},push_bytes(pe),
                       uint32_t((uint64_t(hidden)*emb_ids.size()+255u)/256u),1,1});
        ops.push_back({"segmented_lmhead",join_path_p8c(shader_dir,"p8c_lmhead_q6k_segmented_probe.spv"),
                       {&b_out0,&b_out1,&b_lm_rows,&b_x,&b_lm_y},push_bytes(pl),
                       uint32_t((lm_rows.size()+63u)/64u),1,1});
        PreparedChain chain=vk.prepare_chain(ops);
        ChainStats stats=vk.execute_prepared(chain,ops);
        std::vector<float>emb_got(emb_ref.size()),lm_got(lm_ref.size());
        std::memcpy(emb_got.data(),b_emb_y.mapped,emb_got.size()*sizeof(float));
        std::memcpy(lm_got.data(),b_lm_y.mapped,lm_got.size()*sizeof(float));
        Metrics em=compare_vec(emb_ref,emb_got,0.02,0.005);
        Metrics lm=compare_vec(lm_ref,lm_got,0.02,0.005);
        bool boundary_ids=
            emb_ids[3]==emb_boundary-1u&&emb_ids[4]==emb_boundary&&
            lm_rows[3]==out_boundary-1u&&lm_rows[4]==out_boundary;
        bool finite=true;
        for(float v:emb_got)finite=finite&&std::isfinite(v);
        for(float v:lm_got)finite=finite&&std::isfinite(v);
        bool pass=mapping_equivalence_pass&&em.pass&&lm.pass&&boundary_ids&&finite&&stats.dispatch_count==2u&&stats.submit_count==1u;

        std::ofstream o(out,std::ios::binary);
        if(!o)throw std::runtime_error("cannot write P8-C result JSON");
        o<<"{\n";
        o<<"  \"schema\":\"arcllm.p8c.segmented_access_correctness.v1\",\n";
        o<<"  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";
        o<<"  \"target\":{\"tensor_count\":"<<gguf.tensor_count<<",\"hidden\":"<<hidden<<",\"vocab\":"<<vocab<<"},\n";
        o<<"  \"segments\":{\"embedding\":{\"type\":\"Q4_K\",\"boundary\":"<<emb_boundary<<",\"row_bytes\":"<<emb_rb<<",\"segment_bytes\":["<<emb0_bytes<<","<<emb1_bytes<<"]},\"lm_head\":{\"type\":\"Q6_K\",\"boundary\":"<<out_boundary<<",\"row_bytes\":"<<out_rb<<",\"segment_bytes\":["<<out0_bytes<<","<<out1_bytes<<"]}},\n";
        o<<"  \"probe_rows\":{\"embedding\":[";
        for(size_t i=0;i<emb_ids.size();++i){if(i)o<<",";o<<emb_ids[i];}
        o<<"],\"lm_head\":[";
        for(size_t i=0;i<lm_rows.size();++i){if(i)o<<",";o<<lm_rows[i];}
        o<<"],\"boundary_adjacency_pass\":"<<json_bool(boundary_ids)<<"},\n";
        o<<"  \"mapping_equivalence\":{\"embedding_pass\":"<<json_bool(emb_mapping_equivalence_pass)<<",\"lm_head_pass\":"<<json_bool(lm_mapping_equivalence_pass)<<",\"pass\":"<<json_bool(mapping_equivalence_pass)<<"},\n";
        o<<"  \"embedding\":{\"compared_values\":"<<emb_ref.size()<<",\"max_abs\":"<<std::setprecision(12)<<em.max_abs<<",\"rmse\":"<<em.rmse<<",\"pass\":"<<json_bool(em.pass)<<"},\n";
        o<<"  \"lm_head\":{\"compared_values\":"<<lm_ref.size()<<",\"max_abs\":"<<lm.max_abs<<",\"rmse\":"<<lm.rmse<<",\"pass\":"<<json_bool(lm.pass)<<"},\n";
        o<<"  \"execution\":{\"dispatches\":"<<stats.dispatch_count<<",\"submits\":"<<stats.submit_count<<",\"fence_waits\":"<<stats.fence_wait_count<<",\"finite_pass\":"<<json_bool(finite)<<"},\n";
        o<<"  \"gate\":{\"mapping_equivalence_pass\":"<<json_bool(mapping_equivalence_pass)<<",\"embedding_pass\":"<<json_bool(em.pass)<<",\"lm_head_pass\":"<<json_bool(lm.pass)<<",\"boundary_pass\":"<<json_bool(boundary_ids)<<",\"finite_pass\":"<<json_bool(finite)<<",\"exact_two_dispatches_pass\":"<<json_bool(stats.dispatch_count==2u)<<",\"p8c_pass\":"<<json_bool(pass)<<"}\n";
        o<<"}\n";o.close();
        std::cout<<"ArcLLM P8-C segmented embedding/LM-head correctness\n";
        std::cout<<"embedding max_abs="<<em.max_abs<<" rmse="<<em.rmse<<" pass="<<em.pass<<"\n";
        std::cout<<"lm_head max_abs="<<lm.max_abs<<" rmse="<<lm.rmse<<" pass="<<lm.pass<<"\n";
        std::cout<<"dispatches="<<stats.dispatch_count<<" submits="<<stats.submit_count<<"\n";
        std::cout<<"P8-C "<<(pass?"PASS":"FAIL")<<"\n";
        vk.destroy_prepared(chain);
        vk.destroy_buffer(b_lm_y);vk.destroy_buffer(b_emb_y);vk.destroy_buffer(b_x);
        vk.destroy_buffer(b_lm_rows);vk.destroy_buffer(b_emb_ids);
        vk.destroy_buffer(b_out1);vk.destroy_buffer(b_out0);vk.destroy_buffer(b_emb1);vk.destroy_buffer(b_emb0);
        return pass?0:20;
    }catch(const std::exception&e){
        std::ofstream o(out,std::ios::binary);
        if(o)o<<"{\n  \"schema\":\"arcllm.p8c.segmented_access_correctness.v1\",\n  \"status\":\"ERROR\",\n  \"error\":\""<<e.what()<<"\"\n}\n";
        std::cerr<<e.what()<<"\n";return 2;
    }
}
