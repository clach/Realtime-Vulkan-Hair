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
								   attrib.texcoords[2 * index.texcoord_index + 1]);

			if (uniqueVertices.count(v) == 0) {
				uniqueVertices[v] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(v);
			}
			indices.push_back(uniqueVertices[v]);

		//// Loop over faces(polygon)
		//size_t index_offset = 0;
		//for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
		//	int fv = shapes[s].mesh.num_face_vertices[f];

		//	// Loop over vertices in the face.
		//	for (size_t v = 0; v < fv; v++) {
		//		// access to vertex
		//		tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
		//		posIndices.push_back(idx.vertex_index);
		//		norIndices.push_back(idx.normal_index);

		//		tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
		//		tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
		//		tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
		//		positions.push_back(glm::vec3(vx, vy, vz));

		//		tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
		//		tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
		//		tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
		//		normals.push_back(glm::vec3(nx, ny, nz));

		//	}
		//	index_offset += fv;

		//	// per-face material
		//	shapes[s].mesh.material_ids[f];

		//	numTriangles++;
		}
	}
}