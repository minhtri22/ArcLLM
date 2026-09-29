#include <vulkan/vulkan.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
    VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    ai.pApplicationName = "ArcLLM-CORE0C-Preflight";
    ai.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &ai;
    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&ici, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed");

    uint32_t n = 0;
    if (vkEnumeratePhysicalDevices(instance, &n, nullptr) != VK_SUCCESS || n == 0)
        throw std::runtime_error("no Vulkan physical device");
    std::vector<VkPhysicalDevice> devs(n);
    if (vkEnumeratePhysicalDevices(instance, &n, devs.data()) != VK_SUCCESS)
        throw std::runtime_error("vkEnumeratePhysicalDevices failed");

    VkPhysicalDevice phys = devs[0];
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(phys, &props);

    uint32_t nq = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &nq, nullptr);
    std::vector<VkQueueFamilyProperties> qprops(nq);
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &nq, qprops.data());

    int q = -1;
    for (uint32_t i = 0; i < nq; ++i) {
        if ((qprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && qprops[i].queueCount > 0) {
            q = static_cast<int>(i);
            break;
        }
    }
    if (q < 0) throw std::runtime_error("no compute queue");
    if (qprops[static_cast<size_t>(q)].timestampValidBits == 0)
        throw std::runtime_error("compute queue has timestampValidBits=0");

    std::cout << "{\n"
              << "  \"schema\": \"arcllm.core0c.vulkan_timestamp_probe.v0.1\",\n"
              << "  \"status\": \"PASS\",\n"
              << "  \"device_name\": \"" << props.deviceName << "\",\n"
              << "  \"vendor_id\": " << props.vendorID << ",\n"
              << "  \"device_id\": " << props.deviceID << ",\n"
              << "  \"api_version\": " << props.apiVersion << ",\n"
              << "  \"queue_family\": " << q << ",\n"
              << "  \"timestamp_period_ns\": " << props.limits.timestampPeriod << ",\n"
              << "  \"timestamp_valid_bits\": "
              << qprops[static_cast<size_t>(q)].timestampValidBits << "\n"
              << "}\n";
    vkDestroyInstance(instance, nullptr);
    return 0;
}
