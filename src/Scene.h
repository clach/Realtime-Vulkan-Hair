#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/transform.hpp>
#include <chrono>

#include "Model.h"
#include "Strand.h"

using namespace std::chrono;

struct Time {
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
};

// Collider is ellipsoid
struct Collider {
	glm::mat4 transform;
	glm::mat4 inv;
	glm::mat4 invTrans;

	Collider(glm::vec3 trans, glm::vec3 rot, glm::vec3 scale) {
		glm::mat4 transMat = glm::translate(trans);
		glm::mat4 rotXMat = glm::rotate(rot.x, glm::vec3(1.0, 0.0, 0.0));
		glm::mat4 rotYMat = glm::rotate(rot.y, glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 rotZMat = glm::rotate(rot.z, glm::vec3(0.0, 0.0, 1.0));
		glm::mat4 scaleMat = glm::scale(scale);

		this->transform = transMat * rotZMat * rotYMat * rotXMat * scaleMat * glm::mat4(1.0);
		this->inv = glm::inverse(this->transform);
		this->invTrans = glm::transpose(this->inv);
	}
};

class Scene {
private:
    Device* device;
    
    VkBuffer timeBuffer;
    VkDeviceMemory timeBufferMemory;
    Time time;

    void* mappedData;
	void* mappedData2;

    std::vector<Model*> models;
    std::vector<Hair*> hair;

	std::vector<Collider> colliders;
	VkBuffer collidersBuffer;
	VkDeviceMemory collidersBufferMemory;

	high_resolution_clock::time_point startTime = high_resolution_clock::now();

public:
    Scene() = delete;
    Scene(Device* device, std::vector<Collider> colliders);
    ~Scene();

    const std::vector<Model*>& GetModels() const;
    const std::vector<Hair*>& GetHair() const;
    const std::vector<Collider>& GetColliders() const;
    
    void AddModel(Model* model);
    void AddHair(Hair* hair);
    void AddCollider(Collider collider);

	void translateSphere(glm::vec3 translation);

    VkBuffer GetTimeBuffer() const;
    VkBuffer GetCollidersBuffer() const;

	void CreateCollidersBuffer(VkCommandPool commandPool);

    void UpdateTime();
};
