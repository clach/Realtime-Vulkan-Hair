#include "ObjLoader.h"
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION 
#include "tiny_obj_loader.h"

void ObjLoader::LoadObj(std::string filename, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
	vertices.clear();
	indices.clear();

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

	std::unordered_map<Vertex, uint32_t> uniqueVertices;

	// Loop over shapes
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex v = {};

			v.pos = glm::vec3(attrib.vertices[3 * index.vertex_index + 0], 
							  attrib.vertices[3 * index.vertex_index + 1], 
							  attrib.vertices[3 * index.vertex_index + 2]);

			v.nor = glm::vec3(attrib.normals[3 * index.normal_index + 0],
							  attrib.normals[3 * index.normal_index + 1],
							  attrib.normals[3 * index.normal_index + 2]);

			v.texCoord = glm::vec2(attrib.texcoords[2 * index.texcoord_index + 0],
								   1.f - attrib.texcoords[2 * index.texcoord_index + 1]);

			if (uniqueVertices.count(v) == 0) {
				uniqueVertices[v] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(v);
			}
			indices.push_back(uniqueVertices[v]);
		}
	}
}