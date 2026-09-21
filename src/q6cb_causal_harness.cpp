#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>

#include "q6cb_fixture_generator.hpp"

#include <algorithm>
#include <cmath>
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

namespace {

struct Args {
    std::string authorization;
    std::string fixture_id;
    std::string stratum;
    std::string packed_spv;
    std::string expanded_spv;
    std::string out;
    uint32_t n = 0;
    uint32_t rows = 0;
    uint64_t seed = 0;
};

std::string need(int& i, int argc, char** argv, const char* name) {
    if (i + 1 >= argc) throw std::runtime_error(std::string("missing ") + name);
    return std::string(argv[++i]);
}

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        const std::string x = argv[i];
        if (x == "--authorization") a.authorization = need(i, argc, argv, "--authorization");
        else if (x == "--fixture-id") a.fixture_id = need(i, argc, argv, "--fixture-id");
        else if (x == "--stratum") a.stratum = need(i, argc, argv, "--stratum");
        else if (x == "--n") a.n = uint32_t(std::stoul(need(i, argc, argv, "--n")));
        else if (x == "--rows") a.rows = uint32_t(std::stoul(need(i, argc, argv, "--rows")));
        else if (x == "--seed") a.seed = std::stoull(need(i, argc, argv, "--seed"), nullptr, 0);
        else if (x == "--packed-spv") a.packed_spv = need(i, argc, argv, "--packed-spv");
        else if (x == "--expanded-spv") a.expanded_spv = need(i, argc, argv, "--expanded-spv");
        else if (x == "--out") a.out = need(i, argc, argv, "--out");
        else throw std::runtime_error("unknown argument: " + x);
    }
    return a;
}

void require_future_execution_authorization(const std::string& path) {
    if (path.empty())
        throw std::runtime_error("Q6CB scientific execution authorization missing");
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    if (!f || ss.str().find("Q6CB1_SCIENTIFIC_EXECUTION_AUTHORIZED") == std::string::npos)
        throw std::runtime_error("Q6CB scientific execution authorization invalid");
}

std::vector<uint8_t> read_bytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), {});
}

uint64_t fnv1a64(const void* data, size_t n) {
    const auto* p = static_cast<const uint8_t*>(data);
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

std::string hex64(uint64_t v) {
    std::ostringstream o;
    o << std::hex << std::setw(16) << std::setfill('0') << v;
    return o.str();
}

struct Metric {
    bool finite = true;
    double max_abs = 0.0;
    double rmse = 0.0;
};

template<class A, class B>
Metric metric(const std::vector<A>& a, const std::vector<B>& b) {
    if (a.size() != b.size()) throw std::runtime_error("metric shape mismatch");
    Metric m;
    long double ss = 0.0L;
    for (size_t i = 0; i < a.size(); ++i) {
        const double av = double(a[i]);
        const double bv = double(b[i]);
        if (!std::isfinite(av) || !std::isfinite(bv)) m.finite = false;
        const double d = std::abs(av - bv);
        m.max_abs = (std::max)(m.max_abs, d);
        ss += static_cast<long double>(d) * static_cast<long double>(d);
    }
    if (!a.empty())
        m.rmse = std::sqrt(double(ss / static_cast<long double>(a.size())));
    return m;
}

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    VkDeviceSize bytes = 0;
};

struct Pipeline {
    VkShaderModule module = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

struct VkContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queue_family = 0;
    VkCommandPool command_pool = VK_NULL_HANDLE;

    uint32_t memory_type(uint32_t bits, VkMemoryPropertyFlags required) const {
        VkPhysicalDeviceMemoryProperties mp{};
        vkGetPhysicalDeviceMemoryProperties(physical, &mp);
        for (uint32_t i = 0; i < mp.memoryTypeCount; ++i)
            if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & required) == required)
                return i;
        throw std::runtime_error("host-visible coherent memory type unavailable");
    }

    void init() {
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "ArcLLM-Q6CB1";
        app.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo ii{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ii.pApplicationInfo = &app;
        if (vkCreateInstance(&ii, nullptr, &instance) != VK_SUCCESS)
            throw std::runtime_error("vkCreateInstance failed");

        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (count == 0) throw std::runtime_error("no Vulkan physical device");
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(instance, &count, devices.data());

        for (VkPhysicalDevice d : devices) {
            uint32_t qcount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(d, &qcount, nullptr);
            std::vector<VkQueueFamilyProperties> qp(qcount);
            vkGetPhysicalDeviceQueueFamilyProperties(d, &qcount, qp.data());
            for (uint32_t i = 0; i < qcount; ++i) {
                if ((qp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0u) {
                    physical = d;
                    queue_family = i;
                    break;
                }
            }
            if (physical != VK_NULL_HANDLE) break;
        }
        if (physical == VK_NULL_HANDLE) throw std::runtime_error("no compute queue family");

        VkPhysicalDeviceProperties2 p2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        p2.pNext = &subgroup;
        vkGetPhysicalDeviceProperties2(physical, &p2);
        properties = p2.properties;

        const VkSubgroupFeatureFlags required =
            VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_ARITHMETIC_BIT;
        if (subgroup.subgroupSize != 32u)
            throw std::runtime_error("Q6CB requires subgroup size 32");
        if ((subgroup.supportedOperations & required) != required)
            throw std::runtime_error("Q6CB requires subgroup basic+arithmetic");

        const float priority = 1.0f;
        VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qi.queueFamilyIndex = queue_family;
        qi.queueCount = 1;
        qi.pQueuePriorities = &priority;

        VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        di.queueCreateInfoCount = 1;
        di.pQueueCreateInfos = &qi;
        if (vkCreateDevice(physical, &di, nullptr, &device) != VK_SUCCESS)
            throw std::runtime_error("vkCreateDevice failed");
        vkGetDeviceQueue(device, queue_family, 0, &queue);

        VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        ci.queueFamilyIndex = queue_family;
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(device, &ci, nullptr, &command_pool) != VK_SUCCESS)
            throw std::runtime_error("vkCreateCommandPool failed");
    }

    Buffer make_buffer(VkDeviceSize bytes, const void* initial = nullptr) {
        Buffer out;
        out.bytes = bytes;

        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size = bytes;
        bi.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bi, nullptr, &out.buffer) != VK_SUCCESS)
            throw std::runtime_error("vkCreateBuffer failed");

        VkMemoryRequirements mr{};
        vkGetBufferMemoryRequirements(device, out.buffer, &mr);
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize = mr.size;
        ai.memoryTypeIndex = memory_type(
            mr.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(device, &ai, nullptr, &out.memory) != VK_SUCCESS)
            throw std::runtime_error("vkAllocateMemory failed");
        if (vkBindBufferMemory(device, out.buffer, out.memory, 0) != VK_SUCCESS)
            throw std::runtime_error("vkBindBufferMemory failed");
        if (vkMapMemory(device, out.memory, 0, bytes, 0, &out.mapped) != VK_SUCCESS)
            throw std::runtime_error("vkMapMemory failed");
        if (initial) std::memcpy(out.mapped, initial, size_t(bytes));
        else std::memset(out.mapped, 0, size_t(bytes));
        return out;
    }

    void destroy(Buffer& b) {
        if (b.mapped) vkUnmapMemory(device, b.memory);
        if (b.buffer) vkDestroyBuffer(device, b.buffer, nullptr);
        if (b.memory) vkFreeMemory(device, b.memory, nullptr);
        b = {};
    }

    Pipeline make_pipeline(const std::string& spv_path) {
        const std::vector<uint8_t> spv = read_bytes(spv_path);
        if (spv.empty() || (spv.size() % 4u) != 0u)
            throw std::runtime_error("invalid SPIR-V bytes");

        Pipeline p;
        VkShaderModuleCreateInfo mi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        mi.codeSize = spv.size();
        mi.pCode = reinterpret_cast<const uint32_t*>(spv.data());
        if (vkCreateShaderModule(device, &mi, nullptr, &p.module) != VK_SUCCESS)
            throw std::runtime_error("vkCreateShaderModule failed");

        VkDescriptorSetLayoutBinding bindings[3]{};
        for (uint32_t i = 0; i < 3u; ++i) {
            bindings[i].binding = i;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        VkDescriptorSetLayoutCreateInfo dli{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        dli.bindingCount = 3;
        dli.pBindings = bindings;
        if (vkCreateDescriptorSetLayout(device, &dli, nullptr, &p.descriptor_layout) != VK_SUCCESS)
            throw std::runtime_error("vkCreateDescriptorSetLayout failed");

        VkPushConstantRange pc{};
        pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pc.offset = 0;
        pc.size = 3u * sizeof(uint32_t);
        VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pli.setLayoutCount = 1;
        pli.pSetLayouts = &p.descriptor_layout;
        pli.pushConstantRangeCount = 1;
        pli.pPushConstantRanges = &pc;
        if (vkCreatePipelineLayout(device, &pli, nullptr, &p.layout) != VK_SUCCESS)
            throw std::runtime_error("vkCreatePipelineLayout failed");

        VkPipelineShaderStageCreateInfo si{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        si.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        si.module = p.module;
        si.pName = "main";
        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        ci.stage = si;
        ci.layout = p.layout;
        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &p.pipeline) != VK_SUCCESS)
            throw std::runtime_error("vkCreateComputePipelines failed");
        return p;
    }

    void destroy(Pipeline& p) {
        if (p.pipeline) vkDestroyPipeline(device, p.pipeline, nullptr);
        if (p.layout) vkDestroyPipelineLayout(device, p.layout, nullptr);
        if (p.descriptor_layout) vkDestroyDescriptorSetLayout(device, p.descriptor_layout, nullptr);
        if (p.module) vkDestroyShaderModule(device, p.module, nullptr);
        p = {};
    }

    std::vector<float> run_arm(const Pipeline& pipeline,
                               const void* weights,
                               size_t weight_bytes,
                               const std::vector<float>& x,
                               uint32_t n,
                               uint32_t rows,
                               uint32_t row_bytes) {
        Buffer wb = make_buffer(weight_bytes, weights);
        Buffer xb = make_buffer(x.size() * sizeof(float), x.data());
        Buffer yb = make_buffer(size_t(rows) * sizeof(float), nullptr);

        VkDescriptorPoolSize ps{};
        ps.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ps.descriptorCount = 3;
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pi.maxSets = 1;
        pi.poolSizeCount = 1;
        pi.pPoolSizes = &ps;
        VkDescriptorPool pool = VK_NULL_HANDLE;
        if (vkCreateDescriptorPool(device, &pi, nullptr, &pool) != VK_SUCCESS)
            throw std::runtime_error("vkCreateDescriptorPool failed");

        VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        dai.descriptorPool = pool;
        dai.descriptorSetCount = 1;
        dai.pSetLayouts = &pipeline.descriptor_layout;
        VkDescriptorSet set = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(device, &dai, &set) != VK_SUCCESS)
            throw std::runtime_error("vkAllocateDescriptorSets failed");

        VkDescriptorBufferInfo infos[3]{};
        infos[0] = {wb.buffer, 0, wb.bytes};
        infos[1] = {xb.buffer, 0, xb.bytes};
        infos[2] = {yb.buffer, 0, yb.bytes};
        VkWriteDescriptorSet writes[3]{};
        for (uint32_t i = 0; i < 3u; ++i) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = set;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[i].pBufferInfo = &infos[i];
        }
        vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

        VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = command_pool;
        cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cai.commandBufferCount = 1;
        VkCommandBuffer cb = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(device, &cai, &cb) != VK_SUCCESS)
            throw std::runtime_error("vkAllocateCommandBuffers failed");

        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(cb, &bi) != VK_SUCCESS)
            throw std::runtime_error("vkBeginCommandBuffer failed");

        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE,
                                pipeline.layout, 0, 1, &set, 0, nullptr);
        const uint32_t pc[3] = {n, rows, row_bytes};
        vkCmdPushConstants(cb, pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(pc), pc);
        vkCmdDispatch(cb, (rows + 3u) / 4u, 1u, 1u);

        VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        mb.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(cb,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT,
                             0, 1, &mb, 0, nullptr, 0, nullptr);
        if (vkEndCommandBuffer(cb) != VK_SUCCESS)
            throw std::runtime_error("vkEndCommandBuffer failed");

        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        VkFence fence = VK_NULL_HANDLE;
        if (vkCreateFence(device, &fi, nullptr, &fence) != VK_SUCCESS)
            throw std::runtime_error("vkCreateFence failed");
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cb;
        if (vkQueueSubmit(queue, 1, &si, fence) != VK_SUCCESS)
            throw std::runtime_error("vkQueueSubmit failed");
        if (vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
            throw std::runtime_error("vkWaitForFences failed");

        std::vector<float> out(rows);
        std::memcpy(out.data(), yb.mapped, out.size() * sizeof(float));

        vkDestroyFence(device, fence, nullptr);
        vkFreeCommandBuffers(device, command_pool, 1, &cb);
        vkDestroyDescriptorPool(device, pool, nullptr);
        destroy(yb);
        destroy(xb);
        destroy(wb);
        return out;
    }

    ~VkContext() {
        if (device) vkDeviceWaitIdle(device);
        if (command_pool) vkDestroyCommandPool(device, command_pool, nullptr);
        if (device) vkDestroyDevice(device, nullptr);
        if (instance) vkDestroyInstance(instance, nullptr);
    }
};

void emit_metric(std::ostream& o, const Metric& m) {
    o << "{\"finite\":" << (m.finite ? "true" : "false")
      << ",\"max_abs\":" << std::setprecision(17) << m.max_abs
      << ",\"rmse\":" << m.rmse << "}";
}

template<class T>
void emit_vector(std::ostream& o, const std::vector<T>& v) {
    o << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) o << ",";
        o << std::setprecision(17) << double(v[i]);
    }
    o << "]";
}

} // namespace

int main(int argc, char** argv) {
    Args args;
    try {
        args = parse_args(argc, argv);

        // Critical fail-closed boundary: no fixture generation, CPU causal arm,
        // Vulkan initialization, or GPU dispatch may occur before a future
        // separately committed execution authorization is validated.
        require_future_execution_authorization(args.authorization);

        if (args.fixture_id.empty() || args.stratum.empty() ||
            args.packed_spv.empty() || args.expanded_spv.empty() || args.out.empty())
            throw std::runtime_error("fixture/stratum/SPIR-V/output arguments required");

        q6cb::FixtureSpec spec;
        spec.id = args.fixture_id;
        spec.n = args.n;
        spec.rows = args.rows;
        spec.seed = args.seed;
        spec.stratum = q6cb::parse_stratum(args.stratum);
        q6cb::validate_fixture_spec(spec);

        const q6cb::Fixture fixture = q6cb::generate_fixture(spec);
        const std::vector<float> expanded =
            q6cb::expand_q6_rows(fixture.packed, spec.n, spec.rows);

        // CPU arms: R64, S32, T32-CPU.
        const std::vector<double> r64 =
            q6cb::arm_r64(expanded, fixture.x, spec.n, spec.rows);
        const std::vector<float> s32 =
            q6cb::arm_s32(expanded, fixture.x, spec.n, spec.rows);
        const std::vector<float> t32_cpu =
            q6cb::arm_t32_cpu(expanded, fixture.x, spec.n, spec.rows);

        // GPU arms: T32-GPU-PACKED and T32-GPU-EXPANDED.
        VkContext vk;
        vk.init();
        Pipeline packed_pipeline = vk.make_pipeline(args.packed_spv);
        Pipeline expanded_pipeline = vk.make_pipeline(args.expanded_spv);

        const uint32_t row_bytes =
            uint32_t(size_t(spec.n / q6cb::kQ6ValuesPerBlock) * q6cb::kQ6BlockBytes);
        const std::vector<float> t32_gpu_packed =
            vk.run_arm(packed_pipeline,
                       fixture.packed.data(), fixture.packed.size(),
                       fixture.x, spec.n, spec.rows, row_bytes);
        const std::vector<float> t32_gpu_expanded =
            vk.run_arm(expanded_pipeline,
                       expanded.data(), expanded.size() * sizeof(float),
                       fixture.x, spec.n, spec.rows, 0u);

        vk.destroy(expanded_pipeline);
        vk.destroy(packed_pipeline);

        const Metric s32_vs_r64 = metric(s32, r64);
        const Metric t32cpu_vs_r64 = metric(t32_cpu, r64);
        const Metric gpupacked_vs_r64 = metric(t32_gpu_packed, r64);
        const Metric gpuexpanded_vs_r64 = metric(t32_gpu_expanded, r64);
        const Metric topology = metric(t32_cpu, s32);
        const Metric device = metric(t32_gpu_expanded, t32_cpu);
        const Metric packed_path = metric(t32_gpu_packed, t32_gpu_expanded);

        std::ofstream o(args.out, std::ios::binary);
        if (!o) throw std::runtime_error("cannot open output");
        o << "{\n";
        o << "\"schema\":\"arcllm.q6cb1.causal_observation.v0.1\",\n";
        o << "\"classification_performed\":false,\n";
        o << "\"performance_timing\":false,\n";
        o << "\"model_loaded\":false,\n";
        o << "\"fixture\":{\"id\":\"" << spec.id << "\",\"n\":" << spec.n
          << ",\"rows\":" << spec.rows << ",\"seed\":" << spec.seed
          << ",\"stratum\":\"" << q6cb::stratum_name(spec.stratum) << "\"},\n";
        o << "\"device\":{\"name\":\"" << vk.properties.deviceName
          << "\",\"subgroup_size\":" << vk.subgroup.subgroupSize << "},\n";
        o << "\"hashes\":{\"packed_fnv64\":\""
          << hex64(fnv1a64(fixture.packed.data(), fixture.packed.size()))
          << "\",\"x_fnv64\":\""
          << hex64(fnv1a64(fixture.x.data(), fixture.x.size() * sizeof(float)))
          << "\"},\n";
        o << "\"metrics\":{";
        o << "\"S32_vs_R64\":"; emit_metric(o, s32_vs_r64); o << ",";
        o << "\"T32CPU_vs_R64\":"; emit_metric(o, t32cpu_vs_r64); o << ",";
        o << "\"T32GPU_PACKED_vs_R64\":"; emit_metric(o, gpupacked_vs_r64); o << ",";
        o << "\"T32GPU_EXPANDED_vs_R64\":"; emit_metric(o, gpuexpanded_vs_r64); o << ",";
        o << "\"TOPOLOGY_T32CPU_vs_S32\":"; emit_metric(o, topology); o << ",";
        o << "\"DEVICE_GPU_EXPANDED_vs_T32CPU\":"; emit_metric(o, device); o << ",";
        o << "\"PACKED_PATH_GPU_PACKED_vs_GPU_EXPANDED\":"; emit_metric(o, packed_path);
        o << "},\n";
        o << "\"arms\":{";
        o << "\"R64\":"; emit_vector(o, r64); o << ",";
        o << "\"S32\":"; emit_vector(o, s32); o << ",";
        o << "\"T32_CPU\":"; emit_vector(o, t32_cpu); o << ",";
        o << "\"T32_GPU_PACKED\":"; emit_vector(o, t32_gpu_packed); o << ",";
        o << "\"T32_GPU_EXPANDED\":"; emit_vector(o, t32_gpu_expanded);
        o << "},\n";
        o << "\"status\":\"OBSERVATION_COMPLETE_NOT_ADJUDICATED\"\n";
        o << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Q6CB1 harness error: " << e.what() << "\n";
        return 2;
    }
}
