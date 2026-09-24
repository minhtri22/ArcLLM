#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string esc(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '\\') o << "\\\\";
        else if (c == '"') o << "\\\"";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

std::string hex_uuid(const uint8_t* p, size_t n) {
    std::ostringstream o;
    o << std::hex << std::setfill('0');
    for (size_t i = 0; i < n; ++i) o << std::setw(2) << static_cast<unsigned>(p[i]);
    return o.str();
}

const char* unit_name(VkPerformanceCounterUnitKHR v) {
    switch (v) {
        case VK_PERFORMANCE_COUNTER_UNIT_GENERIC_KHR: return "GENERIC";
        case VK_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_KHR: return "PERCENTAGE";
        case VK_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_KHR: return "NANOSECONDS";
        case VK_PERFORMANCE_COUNTER_UNIT_BYTES_KHR: return "BYTES";
        case VK_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_KHR: return "BYTES_PER_SECOND";
        case VK_PERFORMANCE_COUNTER_UNIT_KELVIN_KHR: return "KELVIN";
        case VK_PERFORMANCE_COUNTER_UNIT_WATTS_KHR: return "WATTS";
        case VK_PERFORMANCE_COUNTER_UNIT_VOLTS_KHR: return "VOLTS";
        case VK_PERFORMANCE_COUNTER_UNIT_AMPS_KHR: return "AMPS";
        case VK_PERFORMANCE_COUNTER_UNIT_HERTZ_KHR: return "HERTZ";
        case VK_PERFORMANCE_COUNTER_UNIT_CYCLES_KHR: return "CYCLES";
        default: return "UNKNOWN";
    }
}
const char* scope_name(VkPerformanceCounterScopeKHR v) {
    switch (v) {
        case VK_PERFORMANCE_COUNTER_SCOPE_COMMAND_BUFFER_KHR: return "COMMAND_BUFFER";
        case VK_PERFORMANCE_COUNTER_SCOPE_RENDER_PASS_KHR: return "RENDER_PASS";
        case VK_PERFORMANCE_COUNTER_SCOPE_COMMAND_KHR: return "COMMAND";
        default: return "UNKNOWN";
    }
}
const char* storage_name(VkPerformanceCounterStorageKHR v) {
    switch (v) {
        case VK_PERFORMANCE_COUNTER_STORAGE_INT32_KHR: return "INT32";
        case VK_PERFORMANCE_COUNTER_STORAGE_INT64_KHR: return "INT64";
        case VK_PERFORMANCE_COUNTER_STORAGE_UINT32_KHR: return "UINT32";
        case VK_PERFORMANCE_COUNTER_STORAGE_UINT64_KHR: return "UINT64";
        case VK_PERFORMANCE_COUNTER_STORAGE_FLOAT32_KHR: return "FLOAT32";
        case VK_PERFORMANCE_COUNTER_STORAGE_FLOAT64_KHR: return "FLOAT64";
        default: return "UNKNOWN";
    }
}

bool has_extension(VkPhysicalDevice pd, const char* name) {
    uint32_t n = 0;
    if (vkEnumerateDeviceExtensionProperties(pd, nullptr, &n, nullptr) != VK_SUCCESS) return false;
    std::vector<VkExtensionProperties> xs(n);
    if (n && vkEnumerateDeviceExtensionProperties(pd, nullptr, &n, xs.data()) != VK_SUCCESS) return false;
    for (const auto& x : xs) if (std::string(x.extensionName) == name) return true;
    return false;
}

}

int main(int argc, char** argv) {
    std::string out = "ARCLLM_V1_M3_COUNTER_CAPABILITY_RAW.json";
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--out" && i + 1 < argc) out = argv[++i];
        else { std::cerr << "usage: m3_vulkan_counter_probe [--out file]\n"; return 2; }
    }

    VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    ai.pApplicationName = "ArcLLM-v1-M3-counter-capability";
    ai.applicationVersion = 1;
    ai.pEngineName = "ArcLLM";
    ai.engineVersion = 1;
    ai.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &ai;
    VkInstance inst = VK_NULL_HANDLE;
    VkResult vr = vkCreateInstance(&ci, nullptr, &inst);
    if (vr != VK_SUCCESS) { std::cerr << "vkCreateInstance failed " << vr << "\n"; return 3; }

    try {
        uint32_t pdn = 0;
        if (vkEnumeratePhysicalDevices(inst, &pdn, nullptr) != VK_SUCCESS || pdn == 0)
            throw std::runtime_error("no Vulkan physical devices");
        std::vector<VkPhysicalDevice> pds(pdn);
        if (vkEnumeratePhysicalDevices(inst, &pdn, pds.data()) != VK_SUCCESS)
            throw std::runtime_error("vkEnumeratePhysicalDevices failed");

        VkPhysicalDevice chosen = pds[0];
        VkPhysicalDeviceProperties props{};
        bool exact = false;
        for (auto pd : pds) {
            VkPhysicalDeviceProperties p{};
            vkGetPhysicalDeviceProperties(pd, &p);
            if (p.vendorID == 0x8086 && p.deviceID == 0x64A0) { chosen = pd; props = p; exact = true; break; }
        }
        if (!exact) vkGetPhysicalDeviceProperties(chosen, &props);

        uint32_t qn = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(chosen, &qn, nullptr);
        std::vector<VkQueueFamilyProperties> qps(qn);
        vkGetPhysicalDeviceQueueFamilyProperties(chosen, &qn, qps.data());
        uint32_t qidx = UINT32_MAX;
        for (uint32_t i = 0; i < qn; ++i) {
            if ((qps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && qps[i].queueCount) { qidx = i; break; }
        }
        if (qidx == UINT32_MAX) throw std::runtime_error("no compute queue family");

        const bool ext = has_extension(chosen, VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);
        VkPhysicalDevicePerformanceQueryFeaturesKHR pf{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR};
        VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        f2.pNext = &pf;
        vkGetPhysicalDeviceFeatures2(chosen, &f2);

        VkPhysicalDevicePerformanceQueryPropertiesKHR pp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR};
        VkPhysicalDeviceProperties2 p2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        p2.pNext = &pp;
        vkGetPhysicalDeviceProperties2(chosen, &p2);

        std::vector<VkPerformanceCounterKHR> counters;
        std::vector<VkPerformanceCounterDescriptionKHR> desc;
        uint32_t passes_all = 0;
        if (ext) {
            auto enumCounters = reinterpret_cast<PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR>(
                vkGetInstanceProcAddr(inst, "vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR"));
            auto getPasses = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR>(
                vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR"));
            if (!enumCounters || !getPasses) throw std::runtime_error("KHR performance query extension advertised but entrypoints missing");

            uint32_t cn = 0;
            vr = enumCounters(chosen, qidx, &cn, nullptr, nullptr);
            if (vr != VK_SUCCESS) throw std::runtime_error("counter count enumeration failed");
            counters.resize(cn);
            desc.resize(cn);
            for (uint32_t i = 0; i < cn; ++i) {
                counters[i].sType = VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR;
                desc[i].sType = VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR;
            }
            if (cn) {
                vr = enumCounters(chosen, qidx, &cn, counters.data(), desc.data());
                if (vr != VK_SUCCESS) throw std::runtime_error("counter enumeration failed");
                std::vector<uint32_t> all(cn);
                for (uint32_t i = 0; i < cn; ++i) all[i] = i;
                VkQueryPoolPerformanceCreateInfoKHR pci{VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR};
                pci.queueFamilyIndex = qidx;
                pci.counterIndexCount = cn;
                pci.pCounterIndices = all.data();
                getPasses(chosen, &pci, &passes_all);
            }
        }

        std::ofstream o(out, std::ios::binary);
        if (!o) throw std::runtime_error("cannot open output");
        o << "{\n";
        o << "  \"schema\":\"arcllm.v1.m3.vulkan_counter_capability_raw.v0.1\",\n";
        o << "  \"execution_class\":\"CAPABILITY_ONLY_NO_MODEL_NO_INFERENCE_NO_COUNTER_COLLECTION\",\n";
        o << "  \"device\":{\"name\":\"" << esc(props.deviceName) << "\",\"vendor_id\":" << props.vendorID
          << ",\"device_id\":" << props.deviceID << ",\"driver_version\":" << props.driverVersion
          << ",\"api_version\":" << props.apiVersion << ",\"exact_arc140v_device_id\":" << (exact?"true":"false") << "},\n";
        o << "  \"queue_family\":{\"index\":" << qidx << ",\"queue_count\":" << qps[qidx].queueCount
          << ",\"timestamp_valid_bits\":" << qps[qidx].timestampValidBits << "},\n";
        o << "  \"vk_khr_performance_query\":{\"extension_present\":" << (ext?"true":"false")
          << ",\"performance_counter_query_pools_feature\":" << (pf.performanceCounterQueryPools?"true":"false")
          << ",\"performance_counter_multiple_query_pools_feature\":" << (pf.performanceCounterMultipleQueryPools?"true":"false")
          << ",\"allow_command_buffer_query_copies\":" << (pp.allowCommandBufferQueryCopies?"true":"false")
          << ",\"counter_count\":" << counters.size() << ",\"passes_for_all_counters\":" << passes_all << "},\n";
        o << "  \"counters\":[\n";
        for (size_t i = 0; i < counters.size(); ++i) {
            const auto& c = counters[i];
            const auto& d = desc[i];
            const bool impacted = (d.flags & VK_PERFORMANCE_COUNTER_DESCRIPTION_CONCURRENTLY_IMPACTED_BIT_KHR) != 0;
            const bool perf = (d.flags & VK_PERFORMANCE_COUNTER_DESCRIPTION_PERFORMANCE_IMPACTING_BIT_KHR) != 0;
            o << "    {\"index\":" << i
              << ",\"name\":\"" << esc(d.name) << "\",\"category\":\"" << esc(d.category)
              << "\",\"description\":\"" << esc(d.description) << "\",\"unit\":\"" << unit_name(c.unit)
              << "\",\"scope\":\"" << scope_name(c.scope) << "\",\"storage\":\"" << storage_name(c.storage)
              << "\",\"uuid\":\"" << hex_uuid(c.uuid, VK_UUID_SIZE)
              << "\",\"concurrently_impacted\":" << (impacted?"true":"false")
              << ",\"performance_impacting\":" << (perf?"true":"false") << "}";
            if (i + 1 != counters.size()) o << ",";
            o << "\n";
        }
        o << "  ]\n}\n";
        o.close();

        std::cout << "M3_VULKAN_CAPABILITY_PROBE=PASS\n";
        std::cout << "DEVICE=" << props.deviceName << "\n";
        std::cout << "VK_KHR_PERFORMANCE_QUERY=" << (ext?"true":"false") << "\n";
        std::cout << "PERFORMANCE_QUERY_FEATURE=" << (pf.performanceCounterQueryPools?"true":"false") << "\n";
        std::cout << "COUNTERS=" << counters.size() << "\n";
        std::cout << "PASSES_ALL=" << passes_all << "\n";
        vkDestroyInstance(inst, nullptr);
        return 0;
    } catch (const std::exception& e) {
        vkDestroyInstance(inst, nullptr);
        std::cerr << e.what() << "\n";
        return 4;
    }
}
