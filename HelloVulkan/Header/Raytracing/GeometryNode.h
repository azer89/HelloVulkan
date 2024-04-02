#ifndef RAYTRACING_GEOMETRY_NODE
#define RAYTRACING_GEOMETRY_NODE

#include <cstdint>

struct GeometryNode
{
	uint64_t vertexBufferDeviceAddress = 0;
	uint64_t indexBufferDeviceAddress = 0;
	int32_t textureIndexBaseColor = 0;
	int32_t textureIndexOcclusion = 0;
};

#endif