#pragma once

#include <array>
#include <vulkan/vulkan.h>
#include "QueueFlags.h"
#include "SwapChain.h"

class SwapChain;
class Device {
    friend class Instance;

public:
    SwapChain* CreateSwapChain(VkSurfaceKHR surface, unsigned int numBuffers);
    Instance* GetInstance();
    VkDevice GetVkDevice();
    VkQueue GetQueue(QueueFlags flag);
    unsigned int GetQueueIndex(QueueFlags flag);
    ~Device();

private:
    using Queues = std::array<VkQueue, sizeof(QueueFlags)>;
    
    Device() = delete;
    Device(Instance* instance, VkDevice vkDevice, Queues queues);

    Instance* instance;
    VkDevice vkDevice;
    Queues queues;
};
