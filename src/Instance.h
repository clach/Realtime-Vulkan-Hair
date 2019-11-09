#pragma once

#include <bitset>
#include <vector>
#include <vulkan/vulkan.h>
#include "QueueFlags.h"
#include "Device.h"

extern const bool ENABLE_VALIDATION;

class Instance {

public:
    Instance() = delete;
    Instance(const char* applicationName, unsigned int additionalExtensionCount = 0, const char** additionalExtensions = nullptr);

    VkInstance GetVkInstance();
    VkPhysicalDevice GetPhysicalDevice();
    const QueueFamilyIndices& GetQueueFamilyIndices() const;
    const VkSurfaceCapabilitiesKHR& GetSurfaceCapabilities() const;
    const std::vector<VkSurfaceFormatKHR>& GetSurfaceFormats() const;
    const std::vector<VkPresentModeKHR>& GetPresentModes() const;
    
    uint32_t GetMemoryTypeIndex(uint32_t types, VkMemoryPropertyFlags properties) const;
    VkFormat GetSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    void PickPhysicalDevice(std::vector<const char*> deviceExtensions, QueueFlagBits requiredQueues, VkSurfaceKHR surface = VK_NULL_HANDLE);

    Device* CreateDevice(QueueFlagBits requiredQueues, VkPhysicalDeviceFeatures deviceFeatures);

    ~Instance();

private:

    void initDebugReport();

    VkInstance instance;
    VkDebugReportCallbackEXT debugReportCallback;
    std::vector<const char*> deviceExtensions;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices queueFamilyIndices;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
};
