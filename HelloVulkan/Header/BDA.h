#ifndef BUFFER_DEVICE_ADDRESS
#define BUFFER_DEVICE_ADDRESS

#include <cstdint>

// Buffer Device Address
struct BDA
{
	uint64_t vertexBufferAddress;
	uint64_t indexBufferAddress;
	uint64_t meshDataBufferAddress; // Per-mesh material
	uint64_t _pad;
};

#endif