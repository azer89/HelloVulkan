#ifndef BUFFER_DEVICE_ADDRESS
#define BUFFER_DEVICE_ADDRESS

#include <cstdint>

struct BDA
{
	uint64_t vertexBufferAddress;
	uint64_t indexBufferAddress;
	uint64_t meshDataBufferAddress;
	uint64_t _pad;
};

#endif