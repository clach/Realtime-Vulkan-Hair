#include "Scene.h"
#include "BufferUtils.h"

Scene::Scene(Device* device) : device(device) {
    BufferUtils::CreateBuffer(device, sizeof(Time), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, timeBuffer, timeBufferMemory);
    vkMapMemory(device->GetVkDevice(), timeBufferMemory, 0, sizeof(Time), 0, &mappedData);
    memcpy(mappedData, &time, sizeof(Time));
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

void Scene::CreateCollidersBuffer(VkCommandPool commandPool) {
	BufferUtils::CreateBufferFromData(device, commandPool, colliders.data(), colliders.size() * sizeof(Collider), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, collidersBuffer, collidersBufferMemory);

}


Scene::~Scene() {
    vkUnmapMemory(device->GetVkDevice(), timeBufferMemory);
    vkDestroyBuffer(device->GetVkDevice(), timeBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), timeBufferMemory, nullptr);

	vkDestroyBuffer(device->GetVkDevice(), collidersBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), collidersBufferMemory, nullptr);
}
