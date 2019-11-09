#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace ShaderModule {
    VkShaderModule Create(const std::vector<char>& code, VkDevice logicalDevice);
    VkShaderModule Create(const std::string& filename, VkDevice logicalDevice);
}
