#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::string jesc(const std::string& s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if (c < 0x20) {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c) << std::dec;
                } else {
                    o << char(c);
                }
        }
    }
    return o.str();
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return char(std::tolower(c)); });
    return s;
}

static std::string ver(uint32_t v) {
    std::ostringstream o;
    o << VK_VERSION_MAJOR(v) << "." << VK_VERSION_MINOR(v) << "." << VK_VERSION_PATCH(v);
    return o.str();
}

static std::string hex_bytes(const uint8_t* p, size_t n) {
    std::ostringstream o;
    o << std::hex << std::setfill('0');
    for (size_t i = 0; i < n; ++i) o << std::setw(2) << unsigned(p[i]);
    return o.str();
}

static bool has_ext(const std::set<std::string>& exts, const char* name) {
    return exts.find(name) != exts.end();
}

static const char* jb(VkBool32 v) { return v ? "true" : "false"; }

int main(int argc, char** argv) {
    std::string device_substring = "Arc";
    std::string out_path;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](const char* opt)->std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + opt);
            return argv[++i];
        };
        if (a == "--device-substring") device_substring = need("--device-substring");
        else if (a == "--out") out_path = need("--out");
        else throw std::runtime_error("unknown argument: " + a);
    }
    if (out_path.empty()) throw std::runtime_error("--out is required");

    VkInstance instance = VK_NULL_HANDLE;
    try {
        uint32_t loader_version = VK_API_VERSION_1_0;
        auto enum_instance_version = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
            vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
        if (enum_instance_version) {
            if (enum_instance_version(&loader_version) != VK_SUCCESS)
                throw std::runtime_error("vkEnumerateInstanceVersion failed");
        }
        if (loader_version < VK_API_VERSION_1_2)
            throw std::runtime_error("SA0-CAP requires Vulkan loader >= 1.2");

        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "ArcLLM-SA0-CAP";
        app.applicationVersion = 1;
        app.pEngineName = "ArcLLM";
        app.engineVersion = 1;
        app.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ici.pApplicationInfo = &app;
        VkResult vr = vkCreateInstance(&ici, nullptr, &instance);
        if (vr != VK_SUCCESS) throw std::runtime_error("vkCreateInstance failed");

        uint32_t dev_count = 0;
        if (vkEnumeratePhysicalDevices(instance, &dev_count, nullptr) != VK_SUCCESS || dev_count == 0)
            throw std::runtime_error("no Vulkan physical device");
        std::vector<VkPhysicalDevice> devices(dev_count);
        if (vkEnumeratePhysicalDevices(instance, &dev_count, devices.data()) != VK_SUCCESS)
            throw std::runtime_error("vkEnumeratePhysicalDevices failed");

        VkPhysicalDevice selected = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties selected_legacy{};
        const std::string needle = lower(device_substring);
        for (VkPhysicalDevice d : devices) {
            VkPhysicalDeviceProperties p{};
            vkGetPhysicalDeviceProperties(d, &p);
            if (lower(p.deviceName).find(needle) != std::string::npos) {
                selected = d;
                selected_legacy = p;
                break;
            }
        }
        if (selected == VK_NULL_HANDLE)
            throw std::runtime_error("requested Vulkan device substring not found");
        if (selected_legacy.apiVersion < VK_API_VERSION_1_2)
            throw std::runtime_error("selected device Vulkan API < 1.2");

        uint32_t ext_count = 0;
        if (vkEnumerateDeviceExtensionProperties(selected, nullptr, &ext_count, nullptr) != VK_SUCCESS)
            throw std::runtime_error("device extension enumeration count failed");
        std::vector<VkExtensionProperties> ext_props(ext_count);
        if (ext_count && vkEnumerateDeviceExtensionProperties(selected, nullptr, &ext_count, ext_props.data()) != VK_SUCCESS)
            throw std::runtime_error("device extension enumeration failed");
        std::set<std::string> exts;
        for (const auto& e : ext_props) exts.insert(e.extensionName);

        const bool subgroup_size_control_available =
            selected_legacy.apiVersion >= VK_API_VERSION_1_3 ||
            has_ext(exts, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);

        VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        VkPhysicalDeviceDriverProperties driver{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
        VkPhysicalDeviceIDProperties ids{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        subgroup.pNext = &driver;
        driver.pNext = &ids;
#ifdef VK_EXT_subgroup_size_control
        VkPhysicalDeviceSubgroupSizeControlPropertiesEXT subgroup_control_props{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT};
        if (subgroup_size_control_available) ids.pNext = &subgroup_control_props;
#endif

        VkPhysicalDeviceProperties2 props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        props2.pNext = &subgroup;
        vkGetPhysicalDeviceProperties2(selected, &props2);

        VkPhysicalDevice8BitStorageFeatures f8{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES};
        VkPhysicalDevice16BitStorageFeatures f16{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
        VkPhysicalDeviceShaderFloat16Int8Features f16i8{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES};
        VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures subgroup_ext{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES};
        f8.pNext = &f16;
        f16.pNext = &f16i8;
        f16i8.pNext = &subgroup_ext;
#ifdef VK_EXT_subgroup_size_control
        VkPhysicalDeviceSubgroupSizeControlFeaturesEXT subgroup_control_features{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT};
        if (subgroup_size_control_available) subgroup_ext.pNext = &subgroup_control_features;
#endif
        VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        features2.pNext = &f8;
        vkGetPhysicalDeviceFeatures2(selected, &features2);

        uint32_t q_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(selected, &q_count, nullptr);
        std::vector<VkQueueFamilyProperties> qprops(q_count);
        if (q_count) vkGetPhysicalDeviceQueueFamilyProperties(selected, &q_count, qprops.data());
        int compute_q = -1;
        for (uint32_t i = 0; i < q_count; ++i) {
            if ((qprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && qprops[i].queueCount > 0) {
                compute_q = int(i);
                break;
            }
        }

        VkPhysicalDeviceMemoryProperties mem{};
        vkGetPhysicalDeviceMemoryProperties(selected, &mem);

        std::vector<VkCooperativeMatrixPropertiesKHR> coop_khr;
        bool coop_khr_function = false;
#ifdef VK_KHR_cooperative_matrix
        if (has_ext(exts, VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME)) {
            auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR>(
                vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR"));
            if (fn) {
                coop_khr_function = true;
                uint32_t n = 0;
                VkResult r0 = fn(selected, &n, nullptr);
                if (r0 == VK_SUCCESS && n) {
                    coop_khr.resize(n);
                    for (auto& x : coop_khr) x.sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_KHR;
                    VkResult r1 = fn(selected, &n, coop_khr.data());
                    if (r1 != VK_SUCCESS && r1 != VK_INCOMPLETE)
                        throw std::runtime_error("KHR cooperative matrix property query failed");
                    coop_khr.resize(n);
                }
            }
        }
#endif

        std::vector<VkCooperativeMatrixPropertiesNV> coop_nv;
        bool coop_nv_function = false;
#ifdef VK_NV_cooperative_matrix
        if (has_ext(exts, VK_NV_COOPERATIVE_MATRIX_EXTENSION_NAME)) {
            auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV>(
                vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV"));
            if (fn) {
                coop_nv_function = true;
                uint32_t n = 0;
                VkResult r0 = fn(selected, &n, nullptr);
                if (r0 == VK_SUCCESS && n) {
                    coop_nv.resize(n);
                    for (auto& x : coop_nv) x.sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_NV;
                    VkResult r1 = fn(selected, &n, coop_nv.data());
                    if (r1 != VK_SUCCESS && r1 != VK_INCOMPLETE)
                        throw std::runtime_error("NV cooperative matrix property query failed");
                    coop_nv.resize(n);
                }
            }
        }
#endif

        const auto& p = props2.properties;
        const bool compute_subgroup = (subgroup.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT) != 0;
        const bool baseline_ok =
            p.vendorID == 0x8086u &&
            lower(p.deviceName).find("arc") != std::string::npos &&
            compute_q >= 0 &&
            subgroup.subgroupSize > 0 &&
            compute_subgroup &&
            p.limits.maxComputeWorkGroupInvocations > 0 &&
            p.limits.maxComputeSharedMemorySize > 0 &&
            mem.memoryHeapCount > 0 &&
            mem.memoryTypeCount > 0;

        std::ofstream o(out_path, std::ios::binary);
        if (!o) throw std::runtime_error("cannot open output JSON");
        o << "{\n";
        o << "  \"schema\":\"arcllm.sa0.capability.raw.v0.1\",\n";
        o << "  \"status\":\"" << (baseline_ok ? "PASS_QUERY" : "FAIL_BASELINE_CAPABILITY") << "\",\n";
        o << "  \"scientific_workload\":\"NOT_RUN\",\n";
        o << "  \"model_loaded\":false,\n";
        o << "  \"shader_modules_created\":0,\n";
        o << "  \"compute_pipelines_created\":0,\n";
        o << "  \"dispatches_submitted\":0,\n";
        o << "  \"loader_api_version\":{\"raw\":" << loader_version << ",\"text\":\"" << ver(loader_version) << "\"},\n";
        o << "  \"physical_device_count\":" << dev_count << ",\n";
        o << "  \"device\":{\n";
        o << "    \"name\":\"" << jesc(p.deviceName) << "\",\n";
        o << "    \"vendor_id\":" << p.vendorID << ",\n";
        o << "    \"device_id\":" << p.deviceID << ",\n";
        o << "    \"device_type\":" << int(p.deviceType) << ",\n";
        o << "    \"api_version\":{\"raw\":" << p.apiVersion << ",\"text\":\"" << ver(p.apiVersion) << "\"},\n";
        o << "    \"driver_version_raw\":" << p.driverVersion << ",\n";
        o << "    \"driver_id\":" << int(driver.driverID) << ",\n";
        o << "    \"driver_name\":\"" << jesc(driver.driverName) << "\",\n";
        o << "    \"driver_info\":\"" << jesc(driver.driverInfo) << "\",\n";
        o << "    \"device_uuid\":\"" << hex_bytes(ids.deviceUUID, VK_UUID_SIZE) << "\",\n";
        o << "    \"driver_uuid\":\"" << hex_bytes(ids.driverUUID, VK_UUID_SIZE) << "\",\n";
        o << "    \"device_luid_valid\":" << jb(ids.deviceLUIDValid) << ",\n";
        o << "    \"device_node_mask\":" << ids.deviceNodeMask << "\n";
        o << "  },\n";

        o << "  \"compute_limits\":{\n";
        o << "    \"max_compute_shared_memory_size\":" << p.limits.maxComputeSharedMemorySize << ",\n";
        o << "    \"max_compute_workgroup_invocations\":" << p.limits.maxComputeWorkGroupInvocations << ",\n";
        o << "    \"max_compute_workgroup_count\":[" << p.limits.maxComputeWorkGroupCount[0] << "," << p.limits.maxComputeWorkGroupCount[1] << "," << p.limits.maxComputeWorkGroupCount[2] << "],\n";
        o << "    \"max_compute_workgroup_size\":[" << p.limits.maxComputeWorkGroupSize[0] << "," << p.limits.maxComputeWorkGroupSize[1] << "," << p.limits.maxComputeWorkGroupSize[2] << "],\n";
        o << "    \"timestamp_period_ns\":" << std::setprecision(9) << p.limits.timestampPeriod << "\n";
        o << "  },\n";

        o << "  \"subgroup\":{\n";
        o << "    \"size\":" << subgroup.subgroupSize << ",\n";
        o << "    \"supported_stages\":" << subgroup.supportedStages << ",\n";
        o << "    \"compute_stage_supported\":" << (compute_subgroup ? "true" : "false") << ",\n";
        o << "    \"supported_operations\":" << subgroup.supportedOperations << ",\n";
        o << "    \"basic\":" << jb(subgroup.supportedOperations & VK_SUBGROUP_FEATURE_BASIC_BIT) << ",\n";
        o << "    \"vote\":" << jb(subgroup.supportedOperations & VK_SUBGROUP_FEATURE_VOTE_BIT) << ",\n";
        o << "    \"arithmetic\":" << jb(subgroup.supportedOperations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT) << ",\n";
        o << "    \"ballot\":" << jb(subgroup.supportedOperations & VK_SUBGROUP_FEATURE_BALLOT_BIT) << ",\n";
        o << "    \"shuffle\":" << jb(subgroup.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_BIT) << ",\n";
        o << "    \"shuffle_relative\":" << jb(subgroup.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT) << ",\n";
        o << "    \"clustered\":" << jb(subgroup.supportedOperations & VK_SUBGROUP_FEATURE_CLUSTERED_BIT) << ",\n";
        o << "    \"quad\":" << jb(subgroup.supportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT) << ",\n";
        o << "    \"extended_types\":" << jb(subgroup_ext.shaderSubgroupExtendedTypes) << ",\n";
        o << "    \"size_control_available\":" << (subgroup_size_control_available ? "true" : "false");
#ifdef VK_EXT_subgroup_size_control
        o << ",\n    \"size_control_feature\":" << jb(subgroup_control_features.subgroupSizeControl) << ",\n";
        o << "    \"compute_full_subgroups\":" << jb(subgroup_control_features.computeFullSubgroups) << ",\n";
        o << "    \"min_size\":" << subgroup_control_props.minSubgroupSize << ",\n";
        o << "    \"max_size\":" << subgroup_control_props.maxSubgroupSize << ",\n";
        o << "    \"max_compute_workgroup_subgroups\":" << subgroup_control_props.maxComputeWorkgroupSubgroups << ",\n";
        o << "    \"required_size_stages\":" << subgroup_control_props.requiredSubgroupSizeStages << "\n";
#else
        o << "\n";
#endif
        o << "  },\n";

        o << "  \"scalar_features\":{\n";
        o << "    \"storage_buffer_8bit_access\":" << jb(f8.storageBuffer8BitAccess) << ",\n";
        o << "    \"uniform_and_storage_buffer_8bit_access\":" << jb(f8.uniformAndStorageBuffer8BitAccess) << ",\n";
        o << "    \"storage_push_constant_8\":" << jb(f8.storagePushConstant8) << ",\n";
        o << "    \"storage_buffer_16bit_access\":" << jb(f16.storageBuffer16BitAccess) << ",\n";
        o << "    \"uniform_and_storage_buffer_16bit_access\":" << jb(f16.uniformAndStorageBuffer16BitAccess) << ",\n";
        o << "    \"storage_push_constant_16\":" << jb(f16.storagePushConstant16) << ",\n";
        o << "    \"storage_input_output_16\":" << jb(f16.storageInputOutput16) << ",\n";
        o << "    \"shader_float16\":" << jb(f16i8.shaderFloat16) << ",\n";
        o << "    \"shader_int8\":" << jb(f16i8.shaderInt8) << "\n";
        o << "  },\n";

        o << "  \"queue_families\":[\n";
        for (uint32_t i = 0; i < q_count; ++i) {
            if (i) o << ",\n";
            o << "    {\"index\":" << i
              << ",\"flags\":" << qprops[i].queueFlags
              << ",\"queue_count\":" << qprops[i].queueCount
              << ",\"timestamp_valid_bits\":" << qprops[i].timestampValidBits
              << ",\"compute\":" << ((qprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT) ? "true" : "false") << "}";
        }
        o << "\n  ],\n";
        o << "  \"selected_compute_queue_family\":" << compute_q << ",\n";

        o << "  \"memory\":{\n";
        o << "    \"heap_count\":" << mem.memoryHeapCount << ",\n";
        o << "    \"type_count\":" << mem.memoryTypeCount << ",\n";
        o << "    \"heaps\":[";
        for (uint32_t i = 0; i < mem.memoryHeapCount; ++i) {
            if (i) o << ",";
            o << "{\"index\":" << i << ",\"size_bytes\":" << mem.memoryHeaps[i].size << ",\"flags\":" << mem.memoryHeaps[i].flags << "}";
        }
        o << "],\n";
        o << "    \"types\":[";
        for (uint32_t i = 0; i < mem.memoryTypeCount; ++i) {
            if (i) o << ",";
            o << "{\"index\":" << i << ",\"heap_index\":" << mem.memoryTypes[i].heapIndex << ",\"property_flags\":" << mem.memoryTypes[i].propertyFlags << "}";
        }
        o << "]\n";
        o << "  },\n";

        o << "  \"cooperative_matrix\":{\n";
#ifdef VK_KHR_cooperative_matrix
        o << "    \"khr_extension\":" << (has_ext(exts, VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME) ? "true" : "false") << ",\n";
#else
        o << "    \"khr_extension\":false,\n";
#endif
        o << "    \"khr_query_function\":" << (coop_khr_function ? "true" : "false") << ",\n";
        o << "    \"khr_property_count\":" << coop_khr.size() << ",\n";
        o << "    \"khr_properties\":[";
        for (size_t i = 0; i < coop_khr.size(); ++i) {
            if (i) o << ",";
            const auto& x = coop_khr[i];
            o << "{\"M\":" << x.MSize << ",\"N\":" << x.NSize << ",\"K\":" << x.KSize
              << ",\"A_type\":" << int(x.AType) << ",\"B_type\":" << int(x.BType)
              << ",\"C_type\":" << int(x.CType) << ",\"result_type\":" << int(x.ResultType)
              << ",\"saturating\":" << jb(x.saturatingAccumulation) << ",\"scope\":" << int(x.scope) << "}";
        }
        o << "],\n";
#ifdef VK_NV_cooperative_matrix
        o << "    \"nv_extension\":" << (has_ext(exts, VK_NV_COOPERATIVE_MATRIX_EXTENSION_NAME) ? "true" : "false") << ",\n";
#else
        o << "    \"nv_extension\":false,\n";
#endif
        o << "    \"nv_query_function\":" << (coop_nv_function ? "true" : "false") << ",\n";
        o << "    \"nv_property_count\":" << coop_nv.size() << ",\n";
        o << "    \"nv_properties\":[";
        for (size_t i = 0; i < coop_nv.size(); ++i) {
            if (i) o << ",";
            const auto& x = coop_nv[i];
            o << "{\"M\":" << x.MSize << ",\"N\":" << x.NSize << ",\"K\":" << x.KSize
              << ",\"A_type\":" << int(x.AType) << ",\"B_type\":" << int(x.BType)
              << ",\"C_type\":" << int(x.CType) << ",\"D_type\":" << int(x.DType)
              << ",\"scope\":" << int(x.scope) << "}";
        }
        o << "]\n";
        o << "  },\n";

        o << "  \"device_extensions\":[";
        size_t ei = 0;
        for (const auto& e : exts) {
            if (ei++) o << ",";
            o << "\"" << jesc(e) << "\"";
        }
        o << "],\n";
        o << "  \"baseline_capability_gate\":" << (baseline_ok ? "true" : "false") << "\n";
        o << "}\n";
        o.close();

        vkDestroyInstance(instance, nullptr);
        return baseline_ok ? 0 : 3;
    } catch (const std::exception& e) {
        if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, nullptr);
        try {
            std::ofstream o(out_path, std::ios::binary);
            if (o) {
                o << "{\n  \"schema\":\"arcllm.sa0.capability.raw.v0.1\",\n"
                  << "  \"status\":\"ERROR\",\n"
                  << "  \"scientific_workload\":\"NOT_RUN\",\n"
                  << "  \"model_loaded\":false,\n"
                  << "  \"shader_modules_created\":0,\n"
                  << "  \"compute_pipelines_created\":0,\n"
                  << "  \"dispatches_submitted\":0,\n"
                  << "  \"error\":\"" << jesc(e.what()) << "\"\n}\n";
            }
        } catch (...) {}
        std::cerr << "SA0-CAP error: " << e.what() << "\n";
        return 2;
    }
}
