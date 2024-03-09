#ifndef RAYTRACING_BUILDER
#define RAYTRACING_BUILDER

#include "VulkanContext.h"

class AccelStructure;
class VulkanBuffer;

class RaytracingBuilder
{
public:
	static void CreateBLAS(VulkanContext& ctx, 
		VulkanBuffer* vertexBuffer,
		VulkanBuffer* indexBuffer,
		VulkanBuffer* transformBuffer,
		uint32_t triangleCount,
		uint32_t vertexCount,
		VkDeviceSize vertexStride,
		AccelStructure* blas);

	static void CreateTLAS(VulkanContext& ctx, AccelStructure* tlas);
};

#endif
