#include "Scene.h"
#include "BufferUtils.h"

Scene::Scene(Device* device, VkCommandPool commandPool, std::vector<Collider> colliders) : device(device), colliders(colliders) {
    BufferUtils::CreateBuffer(device, sizeof(Time), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, timeBuffer, timeBufferMemory);
    vkMapMemory(device->GetVkDevice(), timeBufferMemory, 0, sizeof(Time), 0, &mappedData);
    memcpy(mappedData, &time, sizeof(Time));

	BufferUtils::CreateBuffer(device, sizeof(Collider) * this->colliders.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, collidersBuffer, collidersBufferMemory);
	vkMapMemory(device->GetVkDevice(), collidersBufferMemory, 0, sizeof(Collider) * this->colliders.size(), 0, &mappedData2);
	memcpy(mappedData2, this->colliders.data(), sizeof(Collider) * this->colliders.size());

	this->grid = std::vector<GridCell>();
	int d = 64;
	grid.resize(d * d * d, GridCell(glm::ivec3(0), 0));

	BufferUtils::CreateBufferFromData(device, commandPool, grid.data(), grid.size() * sizeof(GridCell), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, gridBuffer, gridBufferMemory);

	//BufferUtils::CreateBuffer(device, sizeof(glm::vec3) * this->grid.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, gridBuffer, gridBufferMemory);
	/*vkMapMemory(device->GetVkDevice(), gridBufferMemory, 0, sizeof(GridCell) * this->grid.size(), 0, &mappedData3);
	memcpy(mappedData3, this->grid.data(), sizeof(GridCell) * this->grid.size());*/
}

const std::vector<Model*>& Scene::GetModels() const {
    return models;
}

const std::vector<Hair*>& Scene::GetHair() const {
  return hair;
}

const std::vector<Collider>& Scene::GetColliders() const {
	return colliders;
}

const std::vector<GridCell>& Scene::GetGrid() const {
	return grid;
}

void Scene::AddModel(Model* model) {
    models.push_back(model);
}

void Scene::AddHair(Hair* hair) {
  this->hair.push_back(hair);
}

void Scene::AddCollider(Collider collider) {
	this->colliders.push_back(collider);
}

void Scene::UpdateTime() {
    high_resolution_clock::time_point currentTime = high_resolution_clock::now();
    duration<float> nextDeltaTime = duration_cast<duration<float>>(currentTime - startTime);
    startTime = currentTime;

    time.deltaTime = nextDeltaTime.count();
    time.totalTime += time.deltaTime;

    memcpy(mappedData, &time, sizeof(Time));
}

VkBuffer Scene::GetTimeBuffer() const {
    return timeBuffer;
}

VkBuffer Scene::GetCollidersBuffer() const {
	return collidersBuffer;
}

VkBuffer Scene::GetGridBuffer() const {
	return gridBuffer;
}

//void Scene::CreateCollidersBuffer(VkCommandPool commandPool) {
	//BufferUtils::CreateBufferFromData(device, commandPool, colliders.data(), colliders.size() * sizeof(Collider), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, collidersBuffer, collidersBufferMemory);
//}

void Scene::translateSphere(glm::vec3 translation) {
	if (this->colliders.size() > 0) {
		glm::mat4 currTransform = this->colliders.at(0).transform;
		glm::mat4 newTransform = glm::translate(currTransform, translation);

		this->colliders.at(0).transform = newTransform;
		glm::mat4 inverse = glm::inverse(newTransform);
		this->colliders.at(0).inv = inverse;
		this->colliders.at(0).invTrans = glm::transpose(inverse);
	}
	//vkMapMemory(device->GetVkDevice(), collidersBufferMemory, 0, sizeof(Collider) * this->colliders.size(), 0, &mappedData2);
	memcpy(mappedData2, this->colliders.data(), sizeof(Collider) * this->colliders.size());
}

void Scene::clearGrid() {
	//vkCmdFillBuffer(this->gridBuffer, this->gridBuffer, 0, );
	/*for (GridCell g : grid) {
		g.density = 0;
		g.velocity = glm::ivec3(0);
	}
	memcpy(mappedData3, this->grid.data(), sizeof(GridCell) * this->grid.size());*/

}


Scene::~Scene() {
    vkUnmapMemory(device->GetVkDevice(), timeBufferMemory);
    vkDestroyBuffer(device->GetVkDevice(), timeBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), timeBufferMemory, nullptr);

	vkDestroyBuffer(device->GetVkDevice(), collidersBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), collidersBufferMemory, nullptr);
}
