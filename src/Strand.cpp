#include <vector>
#include "Strand.h"
#include "BufferUtils.h"

float generateRandomFloat() {
    return rand() / (float)RAND_MAX;
}

Hair::Hair(Device* device, VkCommandPool commandPool, float planeDim) : Model(device, commandPool, {}, {}) {
    std::vector<Strand> strands;
    strands.reserve(NUM_STRANDS);
}

VkBuffer Hair::GetStrandsBuffer() const {
    return strandsBuffer;
}

Hair::~Hair() {
    vkDestroyBuffer(device->GetVkDevice(), strandsBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), strandsBufferMemory, nullptr);
}
