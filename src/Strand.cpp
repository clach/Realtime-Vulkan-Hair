#define TINYOBJLOADER_IMPLEMENTATION 

#include <vector>
#include <iostream>
#include "Strand.h"
#include "BufferUtils.h"
#include "tiny_obj_loader.h"
#include <iostream>
#include "ObjLoader.h"

float generateRandomFloat() {
    return rand() / (float)RAND_MAX;
}


// generates int in range [min, max)
int generateRandomInt(int min, int max) {
	int scale = (max - min) + min;
	if (scale == 0) {
		// TODO: throw exception?
	}
	return rand() % scale;
}


int GeneratePointsOnMesh(std::string filename, std::vector<glm::vec3>& points, std::vector<glm::vec3>& pointNormals) {
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

	srand(8);
	for (int i = 0; i < NUM_STRANDS; i++)
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
		if (u + v >= 1.f) {
			u = 1 - u;
			v = 1 - v;
		}

		glm::vec3 newPos = p1 * u + p2 * v + p3 * (1.f - u - v);
		points.push_back(newPos);
		pointNormals.push_back(n);
	}

//	std::vector<Vertex> vertices;
//	std::vector<uint32_t> indices;
//	int numTriangles = ObjLoader::LoadObj(filename, vertices, indices);
//
//	//for (int i = 0; i < 3; i++)
////	for (int i = 0; i < vertices.size(); i++)
////	{
////		// select vertex at random
////		//int v = generateRandomInt(0, vertices.size());
////		/*points.push_back(vertices[i].pos + 0.01f * vertices[i].nor);
////		pointNormals.push_back(vertices[i].nor);
////*/
////		// select triangle at random
////		// TODO: account for differences in triangle area?
////		//int triangle = generateRandomInt(0, numTriangles);
////		//int index = 3 * triangle;
////		//glm::vec3 p1 = vertices[index].pos;
////		//glm::vec3 p2 = vertices[index + 1].pos;
////		//glm::vec3 p3 = vertices[index + 2].pos;
////		//glm::vec3 n = normals[index]; // just use same normal for each vertex of face, won't matter for simulation
////
////		//float u = generateRandomFloat();
////		//float v = generateRandomFloat();
////		//if (u + v > 1) {
////		//	u = 1 - u;
////		//	v = 1 - v;
////		//}
////
////		//glm::vec3 newPos = p1 * u + p2 * v + p3 * (1 - u - v);
////		//points.push_back(newPos);
////		//pointNormals.push_back(n);
////	}
	//}
	return NUM_STRANDS;
}


Hair::Hair(Device* device, VkCommandPool commandPool, std::string objFilename) : Model(device, commandPool, {}, {}, glm::mat4(1.0)) {
	// Vector of strands
    std::vector<Strand> strands;

	std::vector<glm::vec3> pointsOnMesh;
	std::vector<glm::vec3> pointNormals;
	numStrands = GeneratePointsOnMesh(objFilename, pointsOnMesh, pointNormals);

	for (int i = 0; i < numStrands; i++) {
		Strand currentStrand = Strand();
		float length = 2.5f;

		// initialize curve point position, velocity and correction vector data
		glm::vec3 currPoint = pointsOnMesh[i];
		for (int j = 0; j < NUM_CURVE_POINTS; j++) {
			currentStrand.curvePoints[j] = glm::vec4(currPoint, 1.0);
			currentStrand.curveVels[j] = glm::vec4(0.0, 0.0, -1.0, 0.0);
			currentStrand.correctionVecs[j] = glm::vec4(0.0);
			glm::vec3 dir = pointNormals[i];
			dir[2] -= 2.0f;
			dir[1] += 5.0f;
			dir[0] += 0.05f;
			currPoint += (float)(length / (NUM_CURVE_POINTS - 1.0)) * dir;
		}

		strands.push_back(currentStrand);
	}

	StrandDrawIndirect indirectDraw;
	indirectDraw.vertexCount = numStrands;
	indirectDraw.instanceCount = 1;
	indirectDraw.firstVertex = 0;
	indirectDraw.firstInstance = 0;

	ModelBufferObject modelMatrix;
	modelMatrix.modelMatrix = glm::mat4(1.0);
	modelMatrix.invTransModelMatrix = glm::mat4(1.0);

	// Create buffers
	BufferUtils::CreateBufferFromData(device, commandPool, strands.data(), numStrands * sizeof(Strand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, strandsBuffer, strandsBufferMemory);
	BufferUtils::CreateBufferFromData(device, commandPool, &indirectDraw, sizeof(StrandDrawIndirect), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, numStrandsBuffer, numStrandsBufferMemory);
	BufferUtils::CreateBufferFromData(device, commandPool, &modelMatrix, sizeof(ModelBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, modelBuffer, modelBufferMemory);
}


VkBuffer Hair::GetStrandsBuffer() const {
    return strandsBuffer;
}


VkBuffer Hair::GetNumStrandsBuffer() const {
	return numStrandsBuffer;
}


VkBuffer Hair::GetModelBuffer() const {
	return modelBuffer;
}


int Hair::GetNumStrands() const {
	return numStrands;
}


Hair::~Hair() {
    vkDestroyBuffer(device->GetVkDevice(), strandsBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), strandsBufferMemory, nullptr);

	vkDestroyBuffer(device->GetVkDevice(), numStrandsBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), numStrandsBufferMemory, nullptr);

	vkDestroyBuffer(device->GetVkDevice(), modelBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), modelBufferMemory, nullptr);
}
