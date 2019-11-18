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
	// TODO: Change this to place the first curve point at points sampled on a surface and remaining curve points along the normal
	for (int i = 0; i < NUM_STRANDS; ++i) {
		Strand currentStrand = Strand();
		float length = MIN_LENGTH + (generateRandomFloat() * (MAX_LENGTH - MIN_LENGTH));
		length = 4.0;
			// currentStrand.len = MIN_LENGTH + (generateRandomFloat() * (MAX_LENGTH - MIN_LENGTH));

		// Hard coded curve points based on length calculation
		float currX = -3.0;
		for (int i = 0; i < NUM_CURVE_POINTS; ++i) {
			currentStrand.curvePoints[i] = glm::vec4(currX, 4.0, 0.0, 1.0);
			currentStrand.curveVels[i] = glm::vec4(0.0);
			currentStrand.correctionVecs[i] = glm::vec4(0.0);
			currX += length / (NUM_CURVE_POINTS - 1.0);
			//currX += currentStrand.len / (NUM_CURVE_POINTS - 1.0);
		}

		strands.push_back(currentStrand);
	}

	StrandDrawIndirect indirectDraw;
	indirectDraw.vertexCount = NUM_STRANDS;
	indirectDraw.instanceCount = 1;
	indirectDraw.firstVertex = 0;
	indirectDraw.firstInstance = 0;

	BufferUtils::CreateBufferFromData(device, commandPool, strands.data(), NUM_STRANDS * sizeof(Strand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, strandsBuffer, strandsBufferMemory);
	BufferUtils::CreateBufferFromData(device, commandPool, &indirectDraw, sizeof(StrandDrawIndirect), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, numStrandsBuffer, numStrandsBufferMemory);
}

VkBuffer Hair::GetStrandsBuffer() const {
    return strandsBuffer;
}

VkBuffer Hair::GetNumStrandsBuffer() const {
	return numStrandsBuffer;
}

Hair::~Hair() {
    vkDestroyBuffer(device->GetVkDevice(), strandsBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), strandsBufferMemory, nullptr);

	vkDestroyBuffer(device->GetVkDevice(), numStrandsBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), numStrandsBufferMemory, nullptr);
}
