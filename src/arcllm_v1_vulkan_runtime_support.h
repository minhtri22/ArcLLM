#pragma once
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
        ai.pApplicationName = "ArcLLM-v1";
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
        auto tall0=std::chrono::steady_clock::now();VkCommandPoolCreateInfo pci{};pci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pci.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;pci.queueFamilyIndex=queue_family_;VkCommandPool pool=nullptr;VkResult vr=create_command_pool_(device_,&pci,nullptr,&pool);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateCommandPool failed ArcLLM runtime");VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer cb=nullptr;vr=allocate_command_buffers_(device_,&ai,&cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkAllocateCommandBuffers failed ArcLLM runtime");VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;vr=begin_command_buffer_(cb,&bi);if(vr!=VK_SUCCESS)throw std::runtime_error("vkBeginCommandBuffer failed ArcLLM runtime");ChainStats st{};
        if(cross_submit_compute_barrier){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);st.initial_compute_barrier_count=1;}
        for(size_t oi=0;oi<ops.size();++oi){const auto& op=ops[oi];const auto& p=chain.prepared[oi];cmd_bind_pipeline_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline);cmd_bind_descriptor_sets_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline_layout,0,1,&p.descriptor_set,0,nullptr);if(!op.push.empty())cmd_push_constants_(cb,p.pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,uint32_t(op.push.size()),op.push.data());cmd_dispatch_(cb,op.gx,op.gy,op.gz);++st.dispatch_count;if(oi+1<ops.size()){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);++st.internal_barrier_count;}}
        VkMemoryBarrier hb{};hb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;hb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;hb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&hb,0,nullptr,0,nullptr);st.final_host_barrier_count=1;vr=end_command_buffer_(cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkEndCommandBuffer failed ArcLLM runtime");VkFenceCreateInfo fci{};fci.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;VkFence fence=nullptr;vr=create_fence_(device_,&fci,nullptr,&fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateFence failed ArcLLM runtime");VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;si.commandBufferCount=1;si.pCommandBuffers=&cb;auto ts0=std::chrono::steady_clock::now();vr=queue_submit_(queue_,1,&si,fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkQueueSubmit failed ArcLLM runtime");st.submit_count=1;vr=wait_for_fences_(device_,1,&fence,VK_TRUE,(std::numeric_limits<uint64_t>::max)());if(vr!=VK_SUCCESS)throw std::runtime_error("vkWaitForFences failed ArcLLM runtime");st.fence_wait_count=1;auto ts1=std::chrono::steady_clock::now();st.submit_wait_ms=std::chrono::duration<double,std::milli>(ts1-ts0).count();destroy_fence_(device_,fence,nullptr);destroy_command_pool_(device_,pool,nullptr);st.record_submit_wait_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tall0).count();return st;
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

static uint32_t q4_row_bytes(uint32_t n) {
    if (n % 256u) throw std::runtime_error("Q4_K row width is not a multiple of 256");
    return (n / 256u) * 144u;
}

static uint32_t q6_row_bytes(uint32_t n) {
    if (n % 256u) throw std::runtime_error("Q6_K row width is not a multiple of 256");
    return (n / 256u) * 210u;
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
    throw std::runtime_error("unsupported tensor type in ArcLLM runtime");
}

static void require_quant_dims(const GgufTensorInfo* t, uint64_t d0, uint64_t d1, const char* label) {
    if (!t || (t->ggml_type != 12u && t->ggml_type != 14u) || t->dims.size()!=2 || t->dims[0]!=d0 || t->dims[1]!=d1)
        throw std::runtime_error(std::string("mixed quant tensor contract mismatch: ")+label);
}

static std::string join_path_p8c(const std::string&a,const std::string&b){
    if(a.empty())return b;char c=a.back();return a+((c=='\\'||c=='/')?"":"\\")+b;
}
