#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include "Model.h"

constexpr static unsigned int NUM_STRANDS = 1;
constexpr static unsigned int NUM_CONTROL_POINTS = 3;
constexpr static float MIN_LENGTH = 1.0f;
constexpr static float MAX_LENGTH = 5.0f;
// TODO: Add more parameters of hair strands here as needed and adjust above values

struct Strand {
	// Attributes
	//std::array<glm::vec4, NUM_CONTROL_POINTS> controlPoints;
	glm::vec4 controlPoints[NUM_CONTROL_POINTS];
	// TODO: Add more attributes to describe a strand if needed.  Stiffness? Width? Length?
	
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Strand);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

	static std::array<VkVertexInputAttributeDescription, NUM_CONTROL_POINTS> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, NUM_CONTROL_POINTS> attributeDescriptions = {};
		for (int i = 0; i < NUM_CONTROL_POINTS; ++i) {
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = i;
			attributeDescriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[i].offset = offsetof(Strand, controlPoints[i]);
		}
		// TODO: Add length and other attributes to attribute descriptions if needed

        return attributeDescriptions;
    }
};

struct StrandDrawIndirect {
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

class Hair : public Model {
private:
    VkBuffer strandsBuffer;
	VkBuffer numStrandsBuffer;

    VkDeviceMemory strandsBufferMemory;
    VkDeviceMemory numStrandsBufferMemory;

public:
	// TODO: Constructor should take in geometry to sample hair positions from
    Hair(Device* device, VkCommandPool commandPool);
    VkBuffer GetStrandsBuffer() const;
    VkBuffer GetNumStrandsBuffer() const;
    ~Hair();
};
