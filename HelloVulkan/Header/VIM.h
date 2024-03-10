#ifndef VERTEX_INDEX_MESH
#define VERTEX_INDEX_MESH

#include "glm/glm.hpp"

struct VIM
{
	uint64_t vertexBufferAddress; // V
	uint64_t indexBufferAddress; // I
	uint64_t meshDataBufferAddress; // M
};

#endif