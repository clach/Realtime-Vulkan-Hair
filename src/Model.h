#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

#include "Vertex.h"
#include "Device.h"
#include <glm/gtx/transform.hpp>


struct ModelBufferObject {
	ModelBufferObject() {
		modelMatrix = glm::mat4(glm::vec4(1,0,0,0), glm::vec4(0,1,0,0), glm::vec4(0,0,1,0), glm::vec4(0));
		invTransModelMatrix = glm::mat4(glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 0, 0));
	}

    glm::mat4 modelMatrix;
	glm::mat4 invTransModelMatrix;
};

class Model {
protected:
    Device* device;

    std::vector<Vertex> vertices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    std::vector<uint32_t> indices;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

   /* VkBuffer modelBuffer;
    VkDeviceMemory modelBufferMemory;*/

	void* mappedData;

    ModelBufferObject modelBufferObject;

    VkImage texture = VK_NULL_HANDLE;
    VkImageView textureView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;

public:
    Model() = delete;
    Model(Device* device, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices, glm::mat4 transform);
    virtual ~Model();

    void SetTexture(VkImage texture);

    const std::vector<Vertex>& getVertices() const;

    VkBuffer getVertexBuffer() const;

    const std::vector<uint32_t>& getIndices() const;

    VkBuffer getIndexBuffer() const;

    ModelBufferObject& getModelBufferObject();

	void translateModel(glm::vec3 translation);

    //VkBuffer GetModelBuffer() const;
    VkImageView GetTextureView() const;
    VkSampler GetTextureSampler() const;
};
