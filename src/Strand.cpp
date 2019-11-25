#include <vector>
#include <iostream>
#include "Strand.h"
#include "BufferUtils.h"
#include <iostream>

#include "ObjLoader.h"

float generateRandomFloat() {
    return rand() / (float)RAND_MAX;
}

// generates int in range [min, max)
int generateRandomInt(int min, int max) {
	int scale = (max - min) + min;
	if (scale == 0) {
		// throw exception?
	}
	return rand() % scale;
}

int GeneratePointsOnMesh(std::string filename, std::vector<glm::vec3>& points, std::vector<glm::vec3>& pointNormals) {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	ObjLoader::LoadObj(filename, vertices, indices);

	for (int i = 0; i < vertices.size(); i++)
	{
		// select vertex at random
		//int v = generateRandomInt(0, vertices.size());
		points.push_back(vertices[i].pos);
		pointNormals.push_back(vertices[i].nor);

		// select triangle at random
		// TODO: account for differences in triangle area?
		//int triangle = generateRandomInt(0, numTriangles);
		//int index = 3 * triangle;
		//glm::vec3 p1 = vertices[index].pos;
		//glm::vec3 p2 = vertices[index + 1].pos;
		//glm::vec3 p3 = vertices[index + 2].pos;
		//glm::vec3 n = normals[index]; // just use same normal for each vertex of face, won't matter for simulation

		//float u = generateRandomFloat();
		//float v = generateRandomFloat();
		//if (u + v > 1) {
		//	u = 1 - u;
		//	v = 1 - v;
		//}

		//glm::vec3 newPos = p1 * u + p2 * v + p3 * (1 - u - v);
		//points.push_back(newPos);
		//pointNormals.push_back(n);
	}

	return vertices.size();
}


Hair::Hair(Device* device, VkCommandPool commandPool, std::string objFilename) : Model(device, commandPool, {}, {}, glm::mat4(1.0)) {
	// Vector of strands
    std::vector<Strand> strands;

	std::vector<glm::vec3> pointsOnMesh;
	std::vector<glm::vec3> pointNormals;
	numStrands = GeneratePointsOnMesh(objFilename, pointsOnMesh, pointNormals);

	for (int i = 0; i < numStrands; i++) {
		Strand currentStrand = Strand();
		float length = MIN_LENGTH + (generateRandomFloat() * (MAX_LENGTH - MIN_LENGTH));
		length = 4.0;

		glm::vec3 currPoint = pointsOnMesh[i];
		for (int j = 0; j < NUM_CURVE_POINTS; j++) {
			currentStrand.curvePoints[j] = glm::vec4(currPoint, 1.0);
			currentStrand.curveVels[j] = glm::vec4(0.0, 0.0, 0.0, 0.0);
			currentStrand.correctionVecs[j] = glm::vec4(0.0);
			currPoint += (float)(length / (NUM_CURVE_POINTS - 1.0)) * pointNormals[i];
		}

		strands.push_back(currentStrand);
	}

	StrandDrawIndirect indirectDraw;
	indirectDraw.vertexCount = numStrands;
	indirectDraw.instanceCount = 1;
	indirectDraw.firstVertex = 0;
	indirectDraw.firstInstance = 0;

	BufferUtils::CreateBufferFromData(device, commandPool, strands.data(), numStrands * sizeof(Strand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, strandsBuffer, strandsBufferMemory);
	BufferUtils::CreateBufferFromData(device, commandPool, &indirectDraw, sizeof(StrandDrawIndirect), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, numStrandsBuffer, numStrandsBufferMemory);
}

VkBuffer Hair::GetStrandsBuffer() const {
    return strandsBuffer;
}

VkBuffer Hair::GetNumStrandsBuffer() const {
	return numStrandsBuffer;
}

int Hair::GetNumStrands() const {
	return numStrands;
}

Hair::~Hair() {
    vkDestroyBuffer(device->GetVkDevice(), strandsBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), strandsBufferMemory, nullptr);

	vkDestroyBuffer(device->GetVkDevice(), numStrandsBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), numStrandsBufferMemory, nullptr);
}
