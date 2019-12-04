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

struct GridCell {
	glm::ivec3 velocity;
	int density;
	GridCell(glm::ivec3 vel, int density) {
		this->velocity = vel;
		this->density = density;
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
	void* mappedData3;

    std::vector<Model*> models;
    std::vector<Hair*> hair;

	std::vector<Collider> colliders;
	VkBuffer collidersBuffer;
	VkDeviceMemory collidersBufferMemory;

	//std::vector<glm::vec3> grid;
	std::vector<GridCell> grid;
	VkBuffer gridBuffer;
	VkDeviceMemory gridBufferMemory;

	high_resolution_clock::time_point startTime = high_resolution_clock::now();

public:
    Scene() = delete;
    Scene(Device* device, VkCommandPool commandPool, std::vector<Collider> colliders);
    ~Scene();

    const std::vector<Model*>& GetModels() const;
    const std::vector<Hair*>& GetHair() const;
    const std::vector<Collider>& GetColliders() const;
	//const std::vector<glm::vec3>& GetGrid() const;
	const std::vector<GridCell>& GetGrid() const;
    
    void AddModel(Model* model);
    void AddHair(Hair* hair);
    void AddCollider(Collider collider);

	void translateSphere(glm::vec3 translation);

    VkBuffer GetTimeBuffer() const;
    VkBuffer GetCollidersBuffer() const;
	VkBuffer GetGridBuffer() const;

	//void CreateCollidersBuffer(VkCommandPool commandPool);

    void UpdateTime();
};
