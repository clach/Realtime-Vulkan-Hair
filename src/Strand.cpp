#include <vector>
#include <iostream>
#include "Strand.h"
#include "BufferUtils.h"
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

float generateRandomFloat() {
    return rand() / (float)RAND_MAX;
}

// generates int in range [min, max)
int generateRandomInt(int min, int max) {
	return rand() % (max - min) + min;
}

void GeneratePointsOnMesh(std::string filename, int numPoints, std::vector<glm::vec3>& points, std::vector<glm::vec3>& pointNormals) {
	// TODO: will probably split this into two functions later
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	// triangulate mesh = true
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), 0, true);

	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<tinyobj::index_t> indices;
	int numTriangles = 0;

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {

		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				indices.push_back(idx);

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				positions.push_back(glm::vec3(vx, vy, vz));

				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				normals.push_back(glm::vec3(nx, ny, nz));

			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];

			numTriangles++;
		}
	}

	for (int i = 0; i < numPoints; i++)
	{
		// select triangle at random
		// TODO: account for differences in triangle area?
		int triangle = generateRandomInt(0, numTriangles);
		int index = 3 * triangle;
		glm::vec3 p1 = positions[index];
		glm::vec3 p2 = positions[index + 1];
		glm::vec3 p3 = positions[index + 2];
		glm::vec3 n = normals[index]; // just use same normal for each vertex of face, won't matter for simulation

		float u = generateRandomFloat();
		float v = generateRandomFloat();
		if (u + v > 1) {
			u = 1 - u;
			v = 1 - v;
		}

		glm::vec3 newPos = p1 * u + p2 * v + p3 * (1 - u - v);
		points.push_back(newPos);
		pointNormals.push_back(n);
	}
}


Hair::Hair(Device* device, VkCommandPool commandPool, std::string objFilename) : Model(device, commandPool, {}, {}) {
	// Vector of strands
    std::vector<Strand> strands;
    strands.reserve(NUM_STRANDS);

	std::vector<glm::vec3> pointsOnMesh;
	std::vector<glm::vec3> pointNormals;
	GeneratePointsOnMesh(objFilename, NUM_STRANDS, pointsOnMesh, pointNormals);

	for (int i = 0; i < NUM_STRANDS; i++) {
		Strand currentStrand = Strand();
		float length = MIN_LENGTH + (generateRandomFloat() * (MAX_LENGTH - MIN_LENGTH));
		length = 4.0;

		glm::vec3 currPoint = pointsOnMesh[i];
		for (int j = 0; j < NUM_CURVE_POINTS; j++) {
			currentStrand.curvePoints[j] = glm::vec4(currPoint, 1.0);
			currentStrand.curveVels[j] = glm::vec4(0.0);
			currentStrand.correctionVecs[j] = glm::vec4(0.0);
			currPoint += (float)(length / (NUM_CURVE_POINTS - 1.0)) * pointNormals[i];
		}

		strands.push_back(currentStrand);
	}

	//for (int i = 0; i < NUM_STRANDS; ++i) {
	//	Strand currentStrand = Strand();
	//	//currentStrand.len = MIN_LENGTH + (generateRandomFloat() * (MAX_LENGTH - MIN_LENGTH));
	//	float length = 4.0;

	//	// Hard coded curve points based on length calculation
	//	float currX = -3.0;
	//	for (int i = 0; i < NUM_CURVE_POINTS; ++i) {
	//		currentStrand.curvePoints[i] = glm::vec4(currX, 4.0, 0.0, 1.0);
	//		currentStrand.curveVels[i] = glm::vec4(0.0);
	//		currentStrand.correctionVecs[i] = glm::vec4(0.0);
	//		currX += length / (NUM_CURVE_POINTS - 1.0);
	//		//currX += currentStrand.len / (NUM_CURVE_POINTS - 1.0);
	//	}

	//	strands.push_back(currentStrand);
	//}

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
