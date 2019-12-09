#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include "Model.h"
#include "iostream"

constexpr static unsigned int NUM_STRANDS = 900;
constexpr static unsigned int NUM_CURVE_POINTS = 10;

struct Strand {
	// Attributes
	glm::vec4 curvePoints[NUM_CURVE_POINTS];
	glm::vec4 curveVels[NUM_CURVE_POINTS];
	glm::vec4 correctionVecs[NUM_CURVE_POINTS];

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Strand);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

	static std::array<VkVertexInputAttributeDescription, 3 * NUM_CURVE_POINTS> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3 * NUM_CURVE_POINTS> attributeDescriptions = {};
		for (int i = 0; i < NUM_CURVE_POINTS; ++i) {
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = i;
			attributeDescriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[i].offset = offsetof(Strand, curvePoints[i]);
		}
		for (int i = NUM_CURVE_POINTS; i < 2 * NUM_CURVE_POINTS; ++i) {
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = i;
			attributeDescriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[i].offset = offsetof(Strand, curveVels[i - NUM_CURVE_POINTS]);
		}

		for (int i = 2 * NUM_CURVE_POINTS; i < 3 * NUM_CURVE_POINTS; ++i) {
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = i;
			attributeDescriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[i].offset = offsetof(Strand, correctionVecs[i - 2 * NUM_CURVE_POINTS]);
		}

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
	VkBuffer modelBuffer;

    VkDeviceMemory strandsBufferMemory;
    VkDeviceMemory numStrandsBufferMemory;
	VkDeviceMemory modelBufferMemory;

	int numStrands;

public:
    Hair(Device* device, VkCommandPool commandPool, std::string objFilename);
    VkBuffer GetStrandsBuffer() const;
    VkBuffer GetNumStrandsBuffer() const;
	VkBuffer GetModelBuffer() const;
	int GetNumStrands() const;
    ~Hair();
};
