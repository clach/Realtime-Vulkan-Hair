#pragma once
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include "Vertex.h"

class ObjLoader
{
public:
	static int LoadObj(std::string filename, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
};

