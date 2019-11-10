#include <vector>
#include "Strand.h"
#include "BufferUtils.h"

float generateRandomFloat() {
    return rand() / (float)RAND_MAX;
}

Hair::Hair(Device* device, VkCommandPool commandPool) : Model(device, commandPool, {}, {}) {
	// Vector of strands
    std::vector<Strand> strands;
    strands.reserve(NUM_STRANDS);

	// Create correct number of strands
	// TODO: Change this to place the first control point at points sampled on a surface and remaining control points along the normal
	for (int i = 0; i < NUM_STRANDS; ++i) {
		Strand currentStrand = Strand();
		float length = MIN_LENGTH + (generateRandomFloat() * (MAX_LENGTH - MIN_LENGTH));

		// Hard coded control points based on length calculation
		float currY = 0.0;
		for (int i = 0; i < NUM_CONTROL_POINTS; ++i) {
			currentStrand.controlPoints[i] = glm::vec4(0.0, currY, 0.0, 1.0);
			currY += length / (NUM_CONTROL_POINTS - 1.0);
		}

		strands.push_back(currentStrand);
	}

	BufferUtils::CreateBufferFromData(device, commandPool, strands.data(), NUM_STRANDS * sizeof(Strand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, strandsBuffer, strandsBufferMemory);
}

VkBuffer Hair::GetStrandsBuffer() const {
    return strandsBuffer;
}

Hair::~Hair() {
    vkDestroyBuffer(device->GetVkDevice(), strandsBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), strandsBufferMemory, nullptr);
}
