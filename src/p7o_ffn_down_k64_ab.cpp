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

        get_device_queue_(device_, queue_family_, 0, &queue_);
        if (!queue_) throw std::runtime_error("queue is null");
    }

    ~VkRuntime() { cleanup(); }

    uint32_t physical_device_count() const { return physical_device_count_; }
    uint32_t queue_family() const { return queue_family_; }
    uint32_t memory_type_index() const { return memory_type_index_; }
    uint32_t memory_type_flags() const { return memory_type_flags_; }

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

int main(int argc,char** argv){
 std::string model,shader_dir,out_path="p7g_ab_results.json";try{
  for(int i=1;i<argc;++i){std::string a=argv[i];auto need=[&](const char* f){if(i+1>=argc)throw std::runtime_error(std::string("missing value for ")+f);return std::string(argv[++i]);};if(a=="--model")model=need("--model");else if(a=="--shader-dir")shader_dir=need("--shader-dir");else if(a=="--out")out_path=need("--out");}
  if(model.empty()||shader_dir.empty())throw std::runtime_error("--model and --shader-dir are required");
  GgufReader reader(model);GgufInfo gguf=reader.read();TensorStore store;TensorStoreReport ts=store.inspect(model,gguf);if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)throw std::runtime_error("P1 tensor-store invariants failed");
  const uint32_t hidden=1536,q_heads=12,kv_heads=2,head_dim=128,kv_dim=256,ffn=8960,layers=28,vocab=151936,Q4_K=12,Q6_K=14,F32=0;const uint32_t pp=512,tg=128,max_ctx=640,pos_base=0;
  if(gguf.tensors.size()!=338u)throw std::runtime_error("P7G expects frozen 338-tensor model");auto bc=gguf.scalars.find("qwen2.block_count");if(bc==gguf.scalars.end()||std::stoul(bc->second)!=layers)throw std::runtime_error("block_count mismatch");
  const auto* emb=find_tensor(gguf,"token_embd.weight");const auto* out_norm=find_tensor(gguf,"output_norm.weight");require_dims(emb,Q6_K,hidden,vocab,"token_embd.weight");require_vec(out_norm,F32,hidden,"output_norm.weight");
  struct LRef{const GgufTensorInfo *an,*qw,*kw,*vw,*qb,*kb,*vb,*ow,*fn,*gw,*uw,*dw;};std::vector<LRef> lr(layers);auto nm=[](uint32_t l,const char* s){return std::string("blk.")+std::to_string(l)+s;};
  for(uint32_t l=0;l<layers;++l){auto A=nm(l,".attn_norm.weight"),QW=nm(l,".attn_q.weight"),KW=nm(l,".attn_k.weight"),VW=nm(l,".attn_v.weight"),QB=nm(l,".attn_q.bias"),KB=nm(l,".attn_k.bias"),VB=nm(l,".attn_v.bias"),OW=nm(l,".attn_output.weight"),FN=nm(l,".ffn_norm.weight"),GW=nm(l,".ffn_gate.weight"),UW=nm(l,".ffn_up.weight"),DW=nm(l,".ffn_down.weight");lr[l]={find_tensor(gguf,A),find_tensor(gguf,QW),find_tensor(gguf,KW),find_tensor(gguf,VW),find_tensor(gguf,QB),find_tensor(gguf,KB),find_tensor(gguf,VB),find_tensor(gguf,OW),find_tensor(gguf,FN),find_tensor(gguf,GW),find_tensor(gguf,UW),find_tensor(gguf,DW)};require_vec(lr[l].an,F32,hidden,A.c_str());require_dims(lr[l].qw,Q4_K,hidden,hidden,QW.c_str());require_dims(lr[l].kw,Q4_K,hidden,kv_dim,KW.c_str());require_quant_dims(lr[l].vw,hidden,kv_dim,VW.c_str());require_vec(lr[l].qb,F32,hidden,QB.c_str());require_vec(lr[l].kb,F32,kv_dim,KB.c_str());require_vec(lr[l].vb,F32,kv_dim,VB.c_str());require_dims(lr[l].ow,Q4_K,hidden,hidden,OW.c_str());require_vec(lr[l].fn,F32,hidden,FN.c_str());require_dims(lr[l].gw,Q4_K,hidden,ffn,GW.c_str());require_dims(lr[l].uw,Q4_K,hidden,ffn,UW.c_str());require_quant_dims(lr[l].dw,ffn,hidden,DW.c_str());}
  float eps=1e-6f;auto ei=gguf.scalars.find("qwen2.attention.layer_norm_rms_epsilon");if(ei!=gguf.scalars.end())try{eps=std::stof(ei->second);}catch(...){}float theta=1000000.0f;auto ri=gguf.scalars.find("qwen2.rope.freq_base");if(ri!=gguf.scalars.end())try{theta=std::stof(ri->second);}catch(...){}
  std::cout<<"ArcLLM P7-O FFN gateup_fused-dequant A/B\n";std::cout<<"layers="<<layers<<" hidden="<<hidden<<" pp="<<pp<<" tg="<<tg<<" max_ctx="<<max_ctx<<"\n";
  VkRuntime vk;vk.init();auto tres0=std::chrono::steady_clock::now();std::vector<ArenaPlan> plans=build_tensor_aware_arenas(gguf,ts.tensor_bytes_total);if(plans.size()!=4u)throw std::runtime_error("P7G frozen arena plan expected 4 arenas");std::vector<Buffer> arenas;for(const auto&a:plans)arenas.push_back(vk.make_buffer(a.end-a.start,store.mapped_base()+gguf.data_offset+a.start));double residency_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tres0).count();
  std::map<std::string,TensorLoc> loc;for(const auto&t:gguf.tensors){uint64_t nb=tensor_nbytes(t);bool found=false;for(uint32_t ai=0;ai<uint32_t(plans.size());++ai)if(t.offset>=plans[ai].start&&t.offset+nb<=plans[ai].end){uint64_t base=t.offset-plans[ai].start;if(base>0xffffffffull)throw std::runtime_error("arena base exceeds uint32");loc[t.name]={ai,uint32_t(base),nb};found=true;break;}if(!found)throw std::runtime_error("tensor crosses P7G arena: "+t.name);}if(loc.size()!=338u)throw std::runtime_error("resident tensor count mismatch");auto L=[&](const std::string&n){auto it=loc.find(n);if(it==loc.end())throw std::runtime_error("missing resident tensor: "+n);return it->second;};auto AB=[&](const std::string&n)->Buffer*{auto z=L(n);return &arenas[z.arena];};auto BASE=[&](const std::string&n){return L(n).byte_base;};auto FBASE=[&](const std::string&n){uint32_t b=BASE(n);if(b%4u)throw std::runtime_error("unaligned F32 tensor");return b/4u;};
  float zero=0.0f;auto fbuf=[&](uint64_t n){return vk.make_buffer(n*sizeof(float));};struct PCEmb{uint32_t n,batch,row_bytes,w_base_bytes;};struct PCRms{uint32_t n,batch;float eps;uint32_t w_base;};struct PCGemm{uint32_t n,rows,batch,row_bytes,add_bias,w_base_bytes,bias_base;};struct PCFusedGU{uint32_t n,rows,batch,row_bytes,gate_base_bytes,up_base_bytes;};struct PCRope{uint32_t batch,heads,head_dim,pos_base;float theta;};struct PCAttnPrefill{uint32_t q_heads,kv_heads,seq,dim;float scale;};struct PCKV{uint32_t layer,cache_pos_start,batch,max_ctx,kv_dim;};struct PCAttnKV{uint32_t layer,pos,max_ctx,q_heads,kv_heads,dim;float scale;};struct PCN{uint32_t n;};struct PCLM{uint32_t n,row_bytes,row_start,row_count,w_base_bytes,x_base;};
  auto addop=[&](std::vector<DispatchOp>&ops,const std::string&name,const std::string&sh,std::vector<Buffer*> bufs,std::vector<uint8_t>push,uint32_t gx,uint32_t gy=1,uint32_t gz=1){DispatchOp o;o.name=name;o.spv_path=shader_dir+"\\"+sh;o.buffers=std::move(bufs);o.push=std::move(push);o.gx=gx;o.gy=gy;o.gz=gz;ops.push_back(std::move(o));};
  TensorLoc el=L("token_embd.weight");uint32_t qrb=q4_row_bytes(hidden),krb=q4_row_bytes(hidden),orb=q4_row_bytes(hidden),gurb=q4_row_bytes(hidden);const uint32_t lm_chunk=8192,lm_dispatches=(vocab+lm_chunk-1u)/lm_chunk;

  // Regression 1: 2-D Q4/Q6 packed GEMM on real layer-0 weights.
  std::vector<float> rx(uint64_t(4)*hidden);for(size_t i=0;i<rx.size();++i)rx[i]=0.2f*std::sin(float(i+1)*0.013f);Buffer brx=vk.make_buffer(rx.size()*sizeof(float),rx.data()),bry4=fbuf(uint64_t(4)*hidden),bry6=fbuf(uint64_t(4)*kv_dim),bdummy=vk.make_buffer(sizeof(float),&zero);auto QW0=nm(0,".attn_q.weight"),QB0=nm(0,".attn_q.bias"),VW0=nm(0,".attn_v.weight"),VB0=nm(0,".attn_v.bias");const float* qb0=reinterpret_cast<const float*>(tensor_ptr(store,gguf,lr[0].qb));const float* vb0=reinterpret_cast<const float*>(tensor_ptr(store,gguf,lr[0].vb));auto refq=matmul_q4_cpu(tensor_ptr(store,gguf,lr[0].qw),hidden,hidden,4,rx,qb0);std::vector<float> refv=(lr[0].vw->ggml_type==Q4_K)?matmul_q4_cpu(tensor_ptr(store,gguf,lr[0].vw),hidden,kv_dim,4,rx,vb0):matmul_q6_cpu(tensor_ptr(store,gguf,lr[0].vw),hidden,kv_dim,4,rx,vb0);std::vector<DispatchOp> rops;PCGemm rgq{hidden,hidden,4,qrb,1,BASE(QW0),FBASE(QB0)};addop(rops,"reg_q4","p7_q4k_gemm_2d.spv",{AB(QW0),&brx,AB(QB0),&bry4},push_bytes(rgq),(hidden+63u)/64u,4);uint32_t vrb0=(lr[0].vw->ggml_type==Q4_K)?q4_row_bytes(hidden):q6_row_bytes(hidden);PCGemm rgv{hidden,kv_dim,4,vrb0,1,BASE(VW0),FBASE(VB0)};addop(rops,"reg_q6",lr[0].vw->ggml_type==Q4_K?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv",{AB(VW0),&brx,AB(VB0),&bry6},push_bytes(rgv),(kv_dim+63u)/64u,4);PreparedChain rprep=vk.prepare_chain(rops);auto rst=vk.execute_prepared(rprep,rops,false);std::vector<float> gy4(refq.size()),gy6(refv.size());std::memcpy(gy4.data(),bry4.mapped,gy4.size()*sizeof(float));std::memcpy(gy6.data(),bry6.mapped,gy6.size()*sizeof(float));Metrics mq=compare_vec(refq,gy4,0.02,0.005),mvq=compare_vec(refv,gy6,0.02,0.005);vk.destroy_prepared(rprep);
  // Regression 2: scalable online attention at seq=4 and cached pos=4.
  const uint32_t rs=4,rmc=16;std::vector<float> rq(uint64_t(rs)*hidden),rk(uint64_t(rs)*kv_dim),rv(uint64_t(rs)*kv_dim);for(size_t i=0;i<rq.size();++i)rq[i]=0.1f*std::cos(float(i+3)*0.007f);for(size_t i=0;i<rk.size();++i)rk[i]=0.1f*std::sin(float(i+5)*0.011f);for(size_t i=0;i<rv.size();++i)rv[i]=0.08f*std::cos(float(i+7)*0.017f);auto refa=attention_cpu(rq,rk,rv,rs,q_heads,kv_heads,head_dim);Buffer brq=vk.make_buffer(rq.size()*sizeof(float),rq.data()),brk=vk.make_buffer(rk.size()*sizeof(float),rk.data()),brv=vk.make_buffer(rv.size()*sizeof(float),rv.data()),bra=fbuf(refa.size());PCAttnPrefill rap{q_heads,kv_heads,rs,head_dim,1.0f/std::sqrt(float(head_dim))};std::vector<DispatchOp> aops;addop(aops,"reg_attn_prefill","p7_attention_prefill_online.spv",{&brq,&brk,&brv,&bra},push_bytes(rap),rs*q_heads);PreparedChain ap=vk.prepare_chain(aops);vk.execute_prepared(ap,aops,false);std::vector<float> ga(refa.size());std::memcpy(ga.data(),bra.mapped,ga.size()*sizeof(float));Metrics ma=compare_vec(refa,ga,0.0002,0.00005);vk.destroy_prepared(ap);
  std::vector<float> ck(uint64_t(rmc)*kv_dim,0.0f),cv(uint64_t(rmc)*kv_dim,0.0f),cq(hidden);for(uint32_t t=0;t<4;++t){std::copy(rk.begin()+uint64_t(t)*kv_dim,rk.begin()+uint64_t(t+1)*kv_dim,ck.begin()+uint64_t(t)*kv_dim);std::copy(rv.begin()+uint64_t(t)*kv_dim,rv.begin()+uint64_t(t+1)*kv_dim,cv.begin()+uint64_t(t)*kv_dim);}for(size_t i=0;i<cq.size();++i)cq[i]=0.1f*std::sin(float(i+9)*0.009f);for(uint32_t d=0;d<kv_dim;++d){ck[uint64_t(4)*kv_dim+d]=0.07f*std::cos(float(d+1)*0.02f);cv[uint64_t(4)*kv_dim+d]=0.05f*std::sin(float(d+1)*0.03f);}std::vector<float> cka(uint64_t(layers)*rmc*kv_dim,0.0f),cva(cka.size(),0.0f);std::copy(ck.begin(),ck.end(),cka.begin());std::copy(cv.begin(),cv.end(),cva.begin());auto refc=attention_cache_cpu(cq,cka,cva,0,4,rmc,q_heads,kv_heads,head_dim);Buffer bcq=vk.make_buffer(cq.size()*sizeof(float),cq.data()),bck=vk.make_buffer(cka.size()*sizeof(float),cka.data()),bcv=vk.make_buffer(cva.size()*sizeof(float),cva.data()),bca=fbuf(refc.size());PCAttnKV rak{0,4,rmc,q_heads,kv_heads,head_dim,1.0f/std::sqrt(float(head_dim))};std::vector<DispatchOp> cops;addop(cops,"reg_attn_kv","p7_attention_kv_online.spv",{&bcq,&bck,&bcv,&bca},push_bytes(rak),q_heads);PreparedChain cp=vk.prepare_chain(cops);vk.execute_prepared(cp,cops,false);std::vector<float> gc(refc.size());std::memcpy(gc.data(),bca.mapped,gc.size()*sizeof(float));Metrics mc=compare_vec(refc,gc,0.0002,0.00005);vk.destroy_prepared(cp);bool regression_pass=mq.pass&&mvq.pass&&ma.pass&&mc.pass;std::cout<<"kernel regression q4="<<mq.pass<<" q6/mixed="<<mvq.pass<<" prefill_attn="<<ma.pass<<" kv_attn="<<mc.pass<<"\n";if(!regression_pass)throw std::runtime_error("P7-C inherited baseline regression failed");
  for(Buffer* b:{&bca,&bcv,&bck,&bcq,&bra,&brv,&brk,&brq,&bry6,&bry4,&brx})vk.destroy_buffer(*b);

  // P7-O regression: FFN-down K64 tiled Q4_K/Q6_K on real frozen tensors.
  const uint32_t tb=9;
  uint32_t q4_layer=layers,q6_layer=layers;
  for(uint32_t l=0;l<layers;++l){if(lr[l].dw->ggml_type==Q4_K&&q4_layer==layers)q4_layer=l;if(lr[l].dw->ggml_type==Q6_K&&q6_layer==layers)q6_layer=l;}
  if(q4_layer==layers||q6_layer==layers)throw std::runtime_error("P7-O requires Q4_K and Q6_K ffn_down tensors");
  std::vector<float> tx(uint64_t(tb)*ffn);for(size_t i=0;i<tx.size();++i)tx[i]=0.13f*std::sin(float(i+5)*0.004f);
  auto DW4=nm(q4_layer,".ffn_down.weight"),DW6=nm(q6_layer,".ffn_down.weight");
  auto ref4=matmul_q4_cpu(tensor_ptr(store,gguf,lr[q4_layer].dw),ffn,hidden,tb,tx,nullptr);
  auto ref6=matmul_q6_cpu(tensor_ptr(store,gguf,lr[q6_layer].dw),ffn,hidden,tb,tx,nullptr);
  Buffer btx=vk.make_buffer(tx.size()*sizeof(float),tx.data()),by4=fbuf(ref4.size()),by6=fbuf(ref6.size());
  PCGemm p4{ffn,hidden,tb,q4_row_bytes(ffn),0,BASE(DW4),0},p6{ffn,hidden,tb,q6_row_bytes(ffn),0,BASE(DW6),0};
  std::vector<DispatchOp> ro4;addop(ro4,"reg_ffn_down_q4_k64","p7o_ffn_down_q4k_k64.spv",{AB(DW4),&btx,&bdummy,&by4},push_bytes(p4),(hidden+7u)/8u,(tb+15u)/16u);
  std::vector<DispatchOp> ro6;addop(ro6,"reg_ffn_down_q6_k64","p7o_ffn_down_q6k_k64.spv",{AB(DW6),&btx,&bdummy,&by6},push_bytes(p6),(hidden+7u)/8u,(tb+15u)/16u);
  PreparedChain pr4=vk.prepare_chain(ro4),pr6=vk.prepare_chain(ro6);vk.execute_prepared(pr4,ro4,false);vk.execute_prepared(pr6,ro6,false);
  std::vector<float> got4(ref4.size()),got6(ref6.size());std::memcpy(got4.data(),by4.mapped,got4.size()*sizeof(float));std::memcpy(got6.data(),by6.mapped,got6.size()*sizeof(float));
  Metrics md4=compare_vec(ref4,got4,0.02,0.005),md6=compare_vec(ref6,got6,0.02,0.005);
  vk.destroy_prepared(pr6);vk.destroy_prepared(pr4);vk.destroy_buffer(by6);vk.destroy_buffer(by4);vk.destroy_buffer(btx);
  bool down_k64_regression_pass=md4.pass&&md6.pass;regression_pass=regression_pass&&down_k64_regression_pass;
  std::cout<<"P7-O FFN-down K64 regression q4="<<md4.pass<<" q6="<<md6.pass<<" q4_layer="<<q4_layer<<" q6_layer="<<q6_layer<<"\n";
  if(!down_k64_regression_pass)throw std::runtime_error("P7-O FFN-down K64 regression failed");

  // Production buffers sized for pp512/tg128.
  std::vector<uint32_t> prompt(pp);for(uint32_t i=0;i<pp;++i)prompt[i]=uint32_t((17ull+7919ull*i)%vocab);uint32_t decode_id=0;Buffer b_ids=vk.make_buffer(uint64_t(pp)*sizeof(uint32_t),prompt.data()),b_dec_id=vk.make_buffer(sizeof(uint32_t),&decode_id);auto fb=[&](uint64_t n){return vk.make_buffer(n*sizeof(float));};Buffer b_h0=fb(uint64_t(pp)*hidden),b_h1=fb(uint64_t(pp)*hidden),b_n1=fb(uint64_t(pp)*hidden),b_q=fb(uint64_t(pp)*hidden),b_k=fb(uint64_t(pp)*kv_dim),b_v=fb(uint64_t(pp)*kv_dim),b_qr=fb(uint64_t(pp)*hidden),b_kr=fb(uint64_t(pp)*kv_dim),b_attn=fb(uint64_t(pp)*hidden),b_o=fb(uint64_t(pp)*hidden),b_r1=fb(uint64_t(pp)*hidden),b_n2=fb(uint64_t(pp)*hidden),b_g=fb(uint64_t(pp)*ffn),b_u=fb(uint64_t(pp)*ffn),b_s=fb(uint64_t(pp)*ffn),b_d=fb(uint64_t(pp)*hidden),b_norm=fb(uint64_t(pp)*hidden),b_logits=fb(vocab);uint64_t cache_elems=uint64_t(layers)*max_ctx*kv_dim;Buffer b_kcache=fb(cache_elems),b_vcache=fb(cache_elems);
  auto build_prefill_baseline=[&](){std::vector<DispatchOp> ops;PCEmb pe{hidden,pp,q6_row_bytes(hidden),el.byte_base};addop(ops,"token_embedding","p7_embedding_q6k.spv",{&arenas[el.arena],&b_ids,&b_h0},push_bytes(pe),(pp*hidden+255u)/256u);Buffer*cur=&b_h0;Buffer*nxt=&b_h1;for(uint32_t l=0;l<layers;++l){std::string pr="L"+(l<10?std::string("0"):std::string())+std::to_string(l)+".";auto AN=nm(l,".attn_norm.weight"),QW=nm(l,".attn_q.weight"),KW=nm(l,".attn_k.weight"),VW=nm(l,".attn_v.weight"),QB=nm(l,".attn_q.bias"),KB=nm(l,".attn_k.bias"),VB=nm(l,".attn_v.bias"),OW=nm(l,".attn_output.weight"),FN=nm(l,".ffn_norm.weight"),GW=nm(l,".ffn_gate.weight"),UW=nm(l,".ffn_up.weight"),DW=nm(l,".ffn_down.weight");PCRms rn{hidden,pp,eps,FBASE(AN)};addop(ops,pr+"attn_rmsnorm","p7_rmsnorm_seq.spv",{cur,AB(AN),&b_n1},push_bytes(rn),pp);uint32_t vrb=(lr[l].vw->ggml_type==Q4_K)?q4_row_bytes(hidden):q6_row_bytes(hidden);PCGemm pq{hidden,hidden,pp,qrb,1,BASE(QW),FBASE(QB)},pk{hidden,kv_dim,pp,krb,1,BASE(KW),FBASE(KB)},pv{hidden,kv_dim,pp,vrb,1,BASE(VW),FBASE(VB)};addop(ops,pr+"q_proj","p7c_ffn_q4k_tiled.spv",{AB(QW),&b_n1,AB(QB),&b_q},push_bytes(pq),(hidden+7u)/8u,(pp+7u)/8u);addop(ops,pr+"k_proj","p7c_ffn_q4k_tiled.spv",{AB(KW),&b_n1,AB(KB),&b_k},push_bytes(pk),(kv_dim+7u)/8u,(pp+7u)/8u);addop(ops,pr+"v_proj",lr[l].vw->ggml_type==Q4_K?"p7c_ffn_q4k_tiled.spv":"p7c_ffn_q6k_tiled.spv",{AB(VW),&b_n1,AB(VB),&b_v},push_bytes(pv),(kv_dim+7u)/8u,(pp+7u)/8u);PCRope qr{pp,q_heads,head_dim,pos_base,theta},kr{pp,kv_heads,head_dim,pos_base,theta};addop(ops,pr+"q_rope","p7_rope_seq.spv",{&b_q,&b_qr},push_bytes(qr),(pp*q_heads*(head_dim/2u)+127u)/128u);addop(ops,pr+"k_rope","p7_rope_seq.spv",{&b_k,&b_kr},push_bytes(kr),(pp*kv_heads*(head_dim/2u)+127u)/128u);PCKV kv{l,0,pp,max_ctx,kv_dim};addop(ops,pr+"kv_store","p7_kv_store.spv",{&b_kr,&b_v,&b_kcache,&b_vcache},push_bytes(kv),(pp*kv_dim+255u)/256u);PCAttnPrefill at{q_heads,kv_heads,pp,head_dim,1.0f/std::sqrt(float(head_dim))};addop(ops,pr+"causal_gqa","p7_attention_prefill_online.spv",{&b_qr,&b_kr,&b_v,&b_attn},push_bytes(at),pp*q_heads);PCGemm po{hidden,hidden,pp,orb,0,BASE(OW),0};addop(ops,pr+"o_proj","p7c_ffn_q4k_tiled.spv",{AB(OW),&b_attn,&bdummy,&b_o},push_bytes(po),(hidden+7u)/8u,(pp+7u)/8u);PCN ph{pp*hidden};addop(ops,pr+"attn_residual","p7_add.spv",{cur,&b_o,&b_r1},push_bytes(ph),(ph.n+255u)/256u);PCRms fn{hidden,pp,eps,FBASE(FN)};addop(ops,pr+"ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b_r1,AB(FN),&b_n2},push_bytes(fn),pp);PCFusedGU pgu{hidden,ffn,pp,gurb,BASE(GW),BASE(UW)};addop(ops,pr+"ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv",{AB(GW),AB(UW),&b_n2,&b_g,&b_u},push_bytes(pgu),(ffn+7u)/8u,(pp+15u)/16u);PCN pf{pp*ffn};addop(ops,pr+"swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(pf),(pf.n+255u)/256u);uint32_t drb=lr[l].dw->ggml_type==Q4_K?q4_row_bytes(ffn):q6_row_bytes(ffn);PCGemm pd{ffn,hidden,pp,drb,0,BASE(DW),0};addop(ops,pr+"ffn_down",lr[l].dw->ggml_type==Q4_K?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv",{AB(DW),&b_s,&bdummy,&b_d},push_bytes(pd),(hidden+7u)/8u,(pp+15u)/16u);addop(ops,pr+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(ph),(ph.n+255u)/256u);std::swap(cur,nxt);}auto ON=std::string("output_norm.weight");PCRms on{hidden,pp,eps,FBASE(ON)};addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB(ON),&b_norm},push_bytes(on),pp);for(uint32_t rs0=0;rs0<vocab;rs0+=lm_chunk){uint32_t rc=(std::min)(lm_chunk,vocab-rs0);PCLM lm{hidden,q6_row_bytes(hidden),rs0,rc,el.byte_base,(pp-1u)*hidden};addop(ops,"lm_head","p7_lmhead_q6k_chunk.spv",{&arenas[el.arena],&b_norm,&b_logits},push_bytes(lm),(rc+63u)/64u);}return ops;};
  auto build_prefill_opt=[&](){std::vector<DispatchOp> ops;PCEmb pe{hidden,pp,q6_row_bytes(hidden),el.byte_base};addop(ops,"token_embedding","p7_embedding_q6k.spv",{&arenas[el.arena],&b_ids,&b_h0},push_bytes(pe),(pp*hidden+255u)/256u);Buffer*cur=&b_h0;Buffer*nxt=&b_h1;for(uint32_t l=0;l<layers;++l){std::string pr="L"+(l<10?std::string("0"):std::string())+std::to_string(l)+".";auto AN=nm(l,".attn_norm.weight"),QW=nm(l,".attn_q.weight"),KW=nm(l,".attn_k.weight"),VW=nm(l,".attn_v.weight"),QB=nm(l,".attn_q.bias"),KB=nm(l,".attn_k.bias"),VB=nm(l,".attn_v.bias"),OW=nm(l,".attn_output.weight"),FN=nm(l,".ffn_norm.weight"),GW=nm(l,".ffn_gate.weight"),UW=nm(l,".ffn_up.weight"),DW=nm(l,".ffn_down.weight");PCRms rn{hidden,pp,eps,FBASE(AN)};addop(ops,pr+"attn_rmsnorm","p7_rmsnorm_seq.spv",{cur,AB(AN),&b_n1},push_bytes(rn),pp);uint32_t vrb=(lr[l].vw->ggml_type==Q4_K)?q4_row_bytes(hidden):q6_row_bytes(hidden);PCGemm pq{hidden,hidden,pp,qrb,1,BASE(QW),FBASE(QB)},pk{hidden,kv_dim,pp,krb,1,BASE(KW),FBASE(KB)},pv{hidden,kv_dim,pp,vrb,1,BASE(VW),FBASE(VB)};addop(ops,pr+"q_proj","p7c_ffn_q4k_tiled.spv",{AB(QW),&b_n1,AB(QB),&b_q},push_bytes(pq),(hidden+7u)/8u,(pp+7u)/8u);addop(ops,pr+"k_proj","p7c_ffn_q4k_tiled.spv",{AB(KW),&b_n1,AB(KB),&b_k},push_bytes(pk),(kv_dim+7u)/8u,(pp+7u)/8u);addop(ops,pr+"v_proj",lr[l].vw->ggml_type==Q4_K?"p7c_ffn_q4k_tiled.spv":"p7c_ffn_q6k_tiled.spv",{AB(VW),&b_n1,AB(VB),&b_v},push_bytes(pv),(kv_dim+7u)/8u,(pp+7u)/8u);PCRope qr{pp,q_heads,head_dim,pos_base,theta},kr{pp,kv_heads,head_dim,pos_base,theta};addop(ops,pr+"q_rope","p7_rope_seq.spv",{&b_q,&b_qr},push_bytes(qr),(pp*q_heads*(head_dim/2u)+127u)/128u);addop(ops,pr+"k_rope","p7_rope_seq.spv",{&b_k,&b_kr},push_bytes(kr),(pp*kv_heads*(head_dim/2u)+127u)/128u);PCKV kv{l,0,pp,max_ctx,kv_dim};addop(ops,pr+"kv_store","p7_kv_store.spv",{&b_kr,&b_v,&b_kcache,&b_vcache},push_bytes(kv),(pp*kv_dim+255u)/256u);PCAttnPrefill at{q_heads,kv_heads,pp,head_dim,1.0f/std::sqrt(float(head_dim))};addop(ops,pr+"causal_gqa","p7_attention_prefill_online.spv",{&b_qr,&b_kr,&b_v,&b_attn},push_bytes(at),pp*q_heads);PCGemm po{hidden,hidden,pp,orb,0,BASE(OW),0};addop(ops,pr+"o_proj","p7c_ffn_q4k_tiled.spv",{AB(OW),&b_attn,&bdummy,&b_o},push_bytes(po),(hidden+7u)/8u,(pp+7u)/8u);PCN ph{pp*hidden};addop(ops,pr+"attn_residual","p7_add.spv",{cur,&b_o,&b_r1},push_bytes(ph),(ph.n+255u)/256u);PCRms fn{hidden,pp,eps,FBASE(FN)};addop(ops,pr+"ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b_r1,AB(FN),&b_n2},push_bytes(fn),pp);PCFusedGU pgu{hidden,ffn,pp,gurb,BASE(GW),BASE(UW)};addop(ops,pr+"ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv",{AB(GW),AB(UW),&b_n2,&b_g,&b_u},push_bytes(pgu),(ffn+7u)/8u,(pp+15u)/16u);PCN pf{pp*ffn};addop(ops,pr+"swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(pf),(pf.n+255u)/256u);uint32_t drb=lr[l].dw->ggml_type==Q4_K?q4_row_bytes(ffn):q6_row_bytes(ffn);PCGemm pd{ffn,hidden,pp,drb,0,BASE(DW),0};addop(ops,pr+"ffn_down",lr[l].dw->ggml_type==Q4_K?"p7o_ffn_down_q4k_k64.spv":"p7o_ffn_down_q6k_k64.spv",{AB(DW),&b_s,&bdummy,&b_d},push_bytes(pd),(hidden+7u)/8u,(pp+15u)/16u);addop(ops,pr+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(ph),(ph.n+255u)/256u);std::swap(cur,nxt);}auto ON=std::string("output_norm.weight");PCRms on{hidden,pp,eps,FBASE(ON)};addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB(ON),&b_norm},push_bytes(on),pp);for(uint32_t rs0=0;rs0<vocab;rs0+=lm_chunk){uint32_t rc=(std::min)(lm_chunk,vocab-rs0);PCLM lm{hidden,q6_row_bytes(hidden),rs0,rc,el.byte_base,(pp-1u)*hidden};addop(ops,"lm_head","p7_lmhead_q6k_chunk.spv",{&arenas[el.arena],&b_norm,&b_logits},push_bytes(lm),(rc+63u)/64u);}return ops;};
  auto build_decode=[&](uint32_t pos){std::vector<DispatchOp> ops;PCEmb pe{hidden,1,q6_row_bytes(hidden),el.byte_base};addop(ops,"token_embedding","p7_embedding_q6k.spv",{&arenas[el.arena],&b_dec_id,&b_h0},push_bytes(pe),(hidden+255u)/256u);Buffer*cur=&b_h0;Buffer*nxt=&b_h1;for(uint32_t l=0;l<layers;++l){std::string pr="L"+(l<10?std::string("0"):std::string())+std::to_string(l)+".";auto AN=nm(l,".attn_norm.weight"),QW=nm(l,".attn_q.weight"),KW=nm(l,".attn_k.weight"),VW=nm(l,".attn_v.weight"),QB=nm(l,".attn_q.bias"),KB=nm(l,".attn_k.bias"),VB=nm(l,".attn_v.bias"),OW=nm(l,".attn_output.weight"),FN=nm(l,".ffn_norm.weight"),GW=nm(l,".ffn_gate.weight"),UW=nm(l,".ffn_up.weight"),DW=nm(l,".ffn_down.weight");PCRms rn{hidden,1,eps,FBASE(AN)};addop(ops,pr+"attn_rmsnorm","p7_rmsnorm_seq.spv",{cur,AB(AN),&b_n1},push_bytes(rn),1);uint32_t vrb=lr[l].vw->ggml_type==Q4_K?q4_row_bytes(hidden):q6_row_bytes(hidden);PCGemm pq{hidden,hidden,1,qrb,1,BASE(QW),FBASE(QB)},pk{hidden,kv_dim,1,krb,1,BASE(KW),FBASE(KB)},pv{hidden,kv_dim,1,vrb,1,BASE(VW),FBASE(VB)};addop(ops,pr+"q_proj","p7_q4k_gemm_2d.spv",{AB(QW),&b_n1,AB(QB),&b_q},push_bytes(pq),(hidden+63u)/64u,1);addop(ops,pr+"k_proj","p7_q4k_gemm_2d.spv",{AB(KW),&b_n1,AB(KB),&b_k},push_bytes(pk),(kv_dim+63u)/64u,1);addop(ops,pr+"v_proj",lr[l].vw->ggml_type==Q4_K?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv",{AB(VW),&b_n1,AB(VB),&b_v},push_bytes(pv),(kv_dim+63u)/64u,1);PCRope qr{1,q_heads,head_dim,pos,theta},kr{1,kv_heads,head_dim,pos,theta};addop(ops,pr+"q_rope","p7_rope_seq.spv",{&b_q,&b_qr},push_bytes(qr),(q_heads*(head_dim/2u)+127u)/128u);addop(ops,pr+"k_rope","p7_rope_seq.spv",{&b_k,&b_kr},push_bytes(kr),(kv_heads*(head_dim/2u)+127u)/128u);PCKV kv{l,pos,1,max_ctx,kv_dim};addop(ops,pr+"kv_store","p7_kv_store.spv",{&b_kr,&b_v,&b_kcache,&b_vcache},push_bytes(kv),(kv_dim+255u)/256u);PCAttnKV at{l,pos,max_ctx,q_heads,kv_heads,head_dim,1.0f/std::sqrt(float(head_dim))};addop(ops,pr+"cached_gqa","p7_attention_kv_online.spv",{&b_qr,&b_kcache,&b_vcache,&b_attn},push_bytes(at),q_heads);PCGemm po{hidden,hidden,1,orb,0,BASE(OW),0};addop(ops,pr+"o_proj","p7_q4k_gemm_2d.spv",{AB(OW),&b_attn,&bdummy,&b_o},push_bytes(po),(hidden+63u)/64u,1);PCN ph{hidden};addop(ops,pr+"attn_residual","p7_add.spv",{cur,&b_o,&b_r1},push_bytes(ph),(hidden+255u)/256u);PCRms fn{hidden,1,eps,FBASE(FN)};addop(ops,pr+"ffn_rmsnorm","p7_rmsnorm_seq.spv",{&b_r1,AB(FN),&b_n2},push_bytes(fn),1);PCGemm pg{hidden,ffn,1,gurb,0,BASE(GW),0},pu{hidden,ffn,1,gurb,0,BASE(UW),0};addop(ops,pr+"ffn_gate","p7_q4k_gemm_2d.spv",{AB(GW),&b_n2,&bdummy,&b_g},push_bytes(pg),(ffn+63u)/64u,1);addop(ops,pr+"ffn_up","p7_q4k_gemm_2d.spv",{AB(UW),&b_n2,&bdummy,&b_u},push_bytes(pu),(ffn+63u)/64u,1);PCN pf{ffn};addop(ops,pr+"swiglu","p7_swiglu.spv",{&b_g,&b_u,&b_s},push_bytes(pf),(ffn+255u)/256u);uint32_t drb=lr[l].dw->ggml_type==Q4_K?q4_row_bytes(ffn):q6_row_bytes(ffn);PCGemm pd{ffn,hidden,1,drb,0,BASE(DW),0};addop(ops,pr+"ffn_down",lr[l].dw->ggml_type==Q4_K?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv",{AB(DW),&b_s,&bdummy,&b_d},push_bytes(pd),(hidden+63u)/64u,1);addop(ops,pr+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(ph),(hidden+255u)/256u);std::swap(cur,nxt);}auto ON=std::string("output_norm.weight");PCRms on{hidden,1,eps,FBASE(ON)};addop(ops,"output_norm","p7_rmsnorm_seq.spv",{cur,AB(ON),&b_norm},push_bytes(on),1);for(uint32_t rs0=0;rs0<vocab;rs0+=lm_chunk){uint32_t rc=(std::min)(lm_chunk,vocab-rs0);PCLM lm{hidden,q6_row_bytes(hidden),rs0,rc,el.byte_base,0};addop(ops,"lm_head","p7_lmhead_q6k_chunk.spv",{&arenas[el.arena],&b_norm,&b_logits},push_bytes(lm),(rc+63u)/64u);}return ops;};
  auto ppbase=build_prefill_baseline();auto ppopt=build_prefill_opt();auto dtemplate=build_decode(pp);const uint32_t expected_prefill=1u+layers*15u+1u+lm_dispatches,expected_decode=1u+layers*16u+1u+lm_dispatches;if(ppbase.size()!=expected_prefill||ppopt.size()!=expected_prefill||dtemplate.size()!=expected_decode)throw std::runtime_error("P7-O dispatch plan mismatch");std::cout<<"Preparing A/B chains: exact P7-L baseline="<<expected_prefill<<" optimized="<<expected_prefill<<"; only FFN-down K tile 32->64; decode="<<expected_decode<<"\n";PreparedChain ppc_base=vk.prepare_chain(ppbase),ppc_opt=vk.prepare_chain(ppopt),dc=vk.prepare_chain(dtemplate);std::cout<<"prepare baseline_ms="<<ppc_base.prepare_ms<<" optimized_ms="<<ppc_opt.prepare_ms<<" decode_ms="<<dc.prepare_ms<<"\n";
  std::vector<double> bwall,bgpu,owall,ogpu;bool allfinite=true,all_top1_match=true,all_logits_match=true;uint32_t first_decode=0,base_top=0,opt_top=0;double worst_logit_max=0.0,worst_logit_rmse=0.0;std::vector<float> base_logits(vocab),opt_logits(vocab);
  auto run_base=[&](){ChainStats st=vk.execute_prepared(ppc_base,ppbase,false);bool fin=true;base_top=argmax_ptr(reinterpret_cast<const float*>(b_logits.mapped),vocab,fin);allfinite=allfinite&&fin;std::memcpy(base_logits.data(),b_logits.mapped,uint64_t(vocab)*sizeof(float));bwall.push_back(st.record_submit_wait_ms);bgpu.push_back(st.submit_wait_ms);return st;};
  auto run_opt=[&](){ChainStats st=vk.execute_prepared(ppc_opt,ppopt,false);bool fin=true;opt_top=argmax_ptr(reinterpret_cast<const float*>(b_logits.mapped),vocab,fin);allfinite=allfinite&&fin;std::memcpy(opt_logits.data(),b_logits.mapped,uint64_t(vocab)*sizeof(float));owall.push_back(st.record_submit_wait_ms);ogpu.push_back(st.submit_wait_ms);return st;};
  for(int trial=0;trial<3;++trial){ChainStats bs{},os{};if(trial==1){os=run_opt();bs=run_base();}else{bs=run_base();os=run_opt();}Metrics lm=compare_vec(base_logits,opt_logits,0.02,0.005);worst_logit_max=(std::max)(worst_logit_max,lm.max_abs);worst_logit_rmse=(std::max)(worst_logit_rmse,lm.rmse);all_logits_match=all_logits_match&&lm.pass;all_top1_match=all_top1_match&&(base_top==opt_top);first_decode=opt_top;std::cout<<"A/B pp512 trial "<<(trial+1)<<" baseline_ms="<<bs.record_submit_wait_ms<<" optimized_ms="<<os.record_submit_wait_ms<<" baseline_top1="<<base_top<<" optimized_top1="<<opt_top<<" logits_pass="<<lm.pass<<"\n";}
  double bmed=median_of(bwall),omed=median_of(owall),bgmed=median_of(bgpu),ogmed=median_of(ogpu);double btps=512000.0/bmed,otps=512000.0/omed,bgtps=512000.0/bgmed,ogtps=512000.0/ogmed;double speedup=bmed/omed,speedup_gpu=bgmed/ogmed;const double min_speedup=1.10;bool perf_pass=speedup>=min_speedup;
  std::memcpy(b_dec_id.mapped,&first_decode,sizeof(first_decode));std::vector<double> stepwall,stepgpu;stepwall.reserve(tg);stepgpu.reserve(tg);std::vector<uint32_t> generated;generated.reserve(tg);auto tg0=std::chrono::steady_clock::now();uint32_t in_tok=first_decode;for(uint32_t ss=0;ss<tg;++ss){uint32_t pos=pp+ss;std::memcpy(b_dec_id.mapped,&in_tok,sizeof(in_tok));auto ops=build_decode(pos);auto step0=std::chrono::steady_clock::now();ChainStats st=vk.execute_prepared(dc,ops,true);bool fin=true;uint32_t out=argmax_ptr(reinterpret_cast<const float*>(b_logits.mapped),vocab,fin);allfinite=allfinite&&fin;auto step1=std::chrono::steady_clock::now();stepwall.push_back(std::chrono::duration<double,std::milli>(step1-step0).count());stepgpu.push_back(st.submit_wait_ms);generated.push_back(out);in_tok=out;if((ss+1)%32u==0u)std::cout<<"decode unchanged "<<(ss+1)<<"/128 last_token="<<out<<"\n";}double tgwall=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tg0).count();double tggpu=0;for(double x:stepgpu)tggpu+=x;double tgtps=128000.0/tgwall,tggtps=128000.0/tggpu;
  const double ref_pp=677.87,ref_tg=40.35,hist_pp=61.90181627,hist_tg=3.723259824;bool invariants=loc.size()==338u&&ts.tensor_bytes_total==980097536ull&&plans.size()==4u&&ppbase.size()==expected_prefill&&ppopt.size()==expected_prefill&&dtemplate.size()==expected_decode;bool correctness_pass=regression_pass&&allfinite&&all_top1_match&&all_logits_match&&generated.size()==tg&&first_decode<vocab;bool pass=correctness_pass&&invariants&&perf_pass;std::cout<<"P7-O pp512 P7L_baseline="<<btps<<" optimized="<<otps<<" speedup="<<speedup<<"x (gate >= "<<min_speedup<<"x); tg128 unchanged="<<tgtps<<" tok/s\n";std::cout<<"P7-O optimization gate="<<(pass?"PASS":"FAIL")<<"; P7 remains OPEN\n";
  std::ofstream o(out_path,std::ios::binary);if(!o)throw std::runtime_error("cannot write result");o<<std::setprecision(10)<<"{\n  \"schema\":\"arcllm.p7o.ffn_down_k64_ab.v1\",\n  \"status\":\""<<(pass?"PASS":"FAIL")<<"\",\n";o<<"  \"model\":{\"layers\":28,\"tensors\":338,\"resident_weight_bytes\":980097536,\"pp_tokens\":512,\"tg_steps\":128,\"max_ctx\":640},\n";o<<"  \"scope\":{\"optimized_family\":\"prefill_ffn_down_k64_only\",\"baseline_prefill_dispatches\":"<<expected_prefill<<",\"optimized_prefill_dispatches\":"<<expected_prefill<<",\"ffn_gate_up_fused_frozen\":true,\"swiglu_separate_frozen\":true,\"baseline_down_k_tile\":32,\"optimized_down_k_tile\":64,\"tile_rows\":8,\"tile_tokens\":16,\"workgroup\":\"8x8\",\"attention_projection_tiling_frozen\":true,\"attention_kernel_changed\":false,\"lm_head_changed\":false,\"decode_gemm_changed\":false},\n";o<<"  \"regression\":{\"inherited_pass\":"<<json_bool(mq.pass&&mvq.pass&&ma.pass&&mc.pass)<<",\"down_q4_k64_max_abs\":"<<md4.max_abs<<",\"down_q4_k64_rmse\":"<<md4.rmse<<",\"down_q4_k64_pass\":"<<json_bool(md4.pass)<<",\"down_q6_k64_max_abs\":"<<md6.max_abs<<",\"down_q6_k64_rmse\":"<<md6.rmse<<",\"down_q6_k64_pass\":"<<json_bool(md6.pass)<<",\"pass\":"<<json_bool(regression_pass)<<"},\n";o<<"  \"ab_pp512\":{\"baseline_trial_wall_ms\":["<<bwall[0]<<","<<bwall[1]<<","<<bwall[2]<<"],\"optimized_trial_wall_ms\":["<<owall[0]<<","<<owall[1]<<","<<owall[2]<<"],\"baseline_median_wall_ms\":"<<bmed<<",\"optimized_median_wall_ms\":"<<omed<<",\"baseline_tok_s\":"<<btps<<",\"optimized_tok_s\":"<<otps<<",\"wall_speedup\":"<<speedup<<",\"submit_wait_speedup\":"<<speedup_gpu<<",\"min_speedup_gate\":"<<min_speedup<<",\"speedup_pass\":"<<json_bool(perf_pass)<<",\"top1_match_all_trials\":"<<json_bool(all_top1_match)<<",\"worst_logits_max_abs\":"<<worst_logit_max<<",\"worst_logits_rmse\":"<<worst_logit_rmse<<",\"logits_gate_max_abs\":0.02,\"logits_gate_rmse\":0.005,\"logits_match_all_trials\":"<<json_bool(all_logits_match)<<",\"historical_p7l_optimized_tok_s\":"<<hist_pp<<",\"reference_tok_s\":"<<ref_pp<<",\"optimized_ratio_to_reference\":"<<(otps/ref_pp)<<"},\n";o<<"  \"tg128_unmodified\":{\"total_wall_ms\":"<<tgwall<<",\"tok_s_wall\":"<<tgtps<<",\"tok_s_submit_wait\":"<<tggtps<<",\"step_wall_median_ms\":"<<median_of(stepwall)<<",\"step_wall_p95_ms\":"<<percentile95(stepwall)<<",\"historical_p7g_decode_tok_s\":"<<hist_tg<<",\"reference_tok_s\":"<<ref_tg<<",\"ratio_to_historical\":"<<(tgtps/hist_tg)<<",\"first_input_token\":"<<first_decode<<",\"last_output_token\":"<<(generated.empty()?0:generated.back())<<"},\n";o<<"  \"p7o_gate\":{\"pass\":"<<json_bool(pass)<<",\"correctness_pass\":"<<json_bool(correctness_pass)<<",\"performance_threshold_pre_registered\":true,\"min_pp512_speedup\":"<<min_speedup<<",\"p7_closed\":false,\"criteria\":\"exact P7-L fused gate+up winner remains baseline; optimized arm changes only prefill FFN-down tiled K from 32 to 64 for both Q4_K and Q6_K while row8, token16, workgroup8x8, gate+up fusion, separate SwiGLU, attention projections, attention kernel, LM head and decode remain unchanged; final logits agree with live baseline; live same-run median pp512 wall speedup >=1.10x and tg128 completes finite\"}\n}\n";o.close();
  vk.destroy_prepared(dc);vk.destroy_prepared(ppc_opt);vk.destroy_prepared(ppc_base);std::vector<Buffer*> scratch={&b_vcache,&b_kcache,&b_logits,&b_norm,&b_d,&b_s,&b_u,&b_g,&b_n2,&b_r1,&b_o,&b_attn,&b_kr,&b_qr,&b_v,&b_k,&b_q,&b_n1,&b_h1,&b_h0,&b_dec_id,&b_ids,&bdummy};for(Buffer*b:scratch)vk.destroy_buffer(*b);for(auto&b:arenas)vk.destroy_buffer(b);return pass?0:20;
 }catch(const std::exception&e){std::cerr<<"P7-O error: "<<e.what()<<"\n";try{std::ofstream o(out_path,std::ios::binary);if(o)o<<"{\n  \"schema\":\"arcllm.p7o.ffn_down_k64_ab.v1\",\n  \"status\":\"ERROR\",\n  \"p7o_gate\":{\"pass\":false,\"p7_closed\":false}\n}\n";}catch(...){}return 2;}
}
