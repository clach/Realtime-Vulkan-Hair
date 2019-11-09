#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include "Model.h"

constexpr static unsigned int NUM_STRANDS = 1;

struct Strand {
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Strand);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};
        return attributeDescriptions;
    }
};

class Hair : public Model {
private:
    VkBuffer strandsBuffer;
    VkDeviceMemory strandsBufferMemory;

public:
    Hair(Device* device, VkCommandPool commandPool, float planeDim);
    VkBuffer GetStrandsBuffer() const;
    ~Hair();
};
