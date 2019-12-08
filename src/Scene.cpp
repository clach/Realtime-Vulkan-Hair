#include "Scene.h"
#include "BufferUtils.h"

Scene::Scene(Device* device, VkCommandPool commandPool, std::vector<Collider> colliders, std::vector<Model*> models) : device(device), colliders(colliders), models(models) {
	// Fill time buffer
	BufferUtils::CreateBuffer(device, sizeof(Time), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, timeBuffer, timeBufferMemory);
    vkMapMemory(device->GetVkDevice(), timeBufferMemory, 0, sizeof(Time), 0, &mappedData);
    memcpy(mappedData, &time, sizeof(Time));

	// Fill colliders buffer
	BufferUtils::CreateBuffer(device, sizeof(Collider) * this->colliders.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, collidersBuffer, collidersBufferMemory);
	vkMapMemory(device->GetVkDevice(), collidersBufferMemory, 0, sizeof(Collider) * this->colliders.size(), 0, &mappedData2);
	memcpy(mappedData2, this->colliders.data(), sizeof(Collider) * this->colliders.size());

	// Fill grid buffer
	this->grid = std::vector<GridCell>();
	int d = 64;
	grid.resize(d * d * d, GridCell(glm::ivec3(0), 0));

	BufferUtils::CreateBufferFromData(device, commandPool, grid.data(), grid.size() * sizeof(GridCell), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, gridBuffer, gridBufferMemory);

	// Fill dynamic model matrix buffer
	size_t minUboAlignment = sizeof(ModelBufferObject);
	dynamicAlignment = sizeof(ModelBufferObject);

	size_t bufferSize = 2 * dynamicAlignment;

	for (int i = 0; i < this->models.size(); ++i) {
		this->modelMatrices.push_back(this->models.at(i)->getModelBufferObject());
	}

	BufferUtils::CreateBuffer(device, this->models.size() * sizeof(ModelBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, modelBuffer, modelBufferMemory);
	vkMapMemory(device->GetVkDevice(), modelBufferMemory, 0, sizeof(ModelBufferObject) * this->models.size(), 0, &mappedData3);
	memcpy(mappedData3, this->modelMatrices.data(), sizeof(ModelBufferObject) * this->models.size());
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


const std::vector<ModelBufferObject>& Scene::GetModelMatrices() const {
	return modelMatrices;
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


VkBuffer Scene::GetModelBuffer() const {
	return modelBuffer;
}


void Scene::translateSphere(glm::vec3 translation) {
	// Update colliders transformation
	if (this->colliders.size() > 0) {
		glm::mat4 currTransform = this->colliders.at(0).transform;
		glm::mat4 newTransform = glm::translate(currTransform, translation);

		this->colliders.at(0).transform = newTransform;
		glm::mat4 inverse = glm::inverse(newTransform);
		this->colliders.at(0).inv = inverse;
		this->colliders.at(0).invTrans = glm::transpose(inverse);
	}
	// Update model matrix transformation
	if (this->models.size() > 0) {
		glm::mat4 currTransform = this->models.at(0)->getModelBufferObject().modelMatrix;
		glm::mat4 newTransform = glm::translate(currTransform, translation);

		this->models.at(0)->getModelBufferObject().modelMatrix = newTransform;
		this->models.at(0)->getModelBufferObject().invTransModelMatrix = glm::inverse(newTransform);

		this->modelMatrices.at(0) = this->models.at(0)->getModelBufferObject();

	}
	// Update buffers
	memcpy(mappedData2, this->colliders.data(), sizeof(Collider) * this->colliders.size());
	memcpy(mappedData3, this->modelMatrices.data(), sizeof(ModelBufferObject) * this->models.size());

}


Scene::~Scene() {
    vkUnmapMemory(device->GetVkDevice(), timeBufferMemory);
    vkDestroyBuffer(device->GetVkDevice(), timeBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), timeBufferMemory, nullptr);

	vkUnmapMemory(device->GetVkDevice(), collidersBufferMemory);
	vkDestroyBuffer(device->GetVkDevice(), collidersBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), collidersBufferMemory, nullptr);

	vkUnmapMemory(device->GetVkDevice(), modelBufferMemory);
	vkDestroyBuffer(device->GetVkDevice(), modelBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), modelBufferMemory, nullptr);
}
