#pragma once

#include <vector>
#include "Device.h"

class Device;
class SwapChain {
    friend class Device;

public:
    VkSwapchainKHR GetVkSwapChain() const;
    VkFormat GetVkImageFormat() const;
    VkExtent2D GetVkExtent() const;
    uint32_t GetIndex() const;
    uint32_t GetCount() const;
    VkImage GetVkImage(uint32_t index) const;
    VkSemaphore GetImageAvailableVkSemaphore() const;
    VkSemaphore GetRenderFinishedVkSemaphore() const;
    
    void Recreate();
    bool Acquire();
    bool Present();
    ~SwapChain();

private:
    SwapChain(Device* device, VkSurfaceKHR vkSurface, unsigned int numBuffers);
    void Create();
    void Destroy();

    Device* device;
    VkSurfaceKHR vkSurface;
    unsigned int numBuffers;
    VkSwapchainKHR vkSwapChain;
    std::vector<VkImage> vkSwapChainImages;
    VkFormat vkSwapChainImageFormat;
    VkExtent2D vkSwapChainExtent;
    uint32_t imageIndex = 0;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
};
