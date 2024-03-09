#ifndef RAYTRACING_BUILDER
#define RAYTRACING_BUILDER

#include "VulkanContext.h"

class AccelStructure;
class VulkanBuffer;

class RaytracingBuilder
{
public:
	static void CreateBLAS(VulkanContext& ctx, 
		const VulkanBuffer& vertexBuffer,
		const VulkanBuffer& indexBuffer,
		const VulkanBuffer& transformBuffer,
		uint32_t triangleCount,
		uint32_t vertexCount,
		VkDeviceSize vertexStride,
		AccelStructure* blas);

	static void CreateTLAS(VulkanContext& ctx, 
		VkTransformMatrixKHR& transformMatrix,
		uint64_t blasDeviceAddress,
		AccelStructure* tlas);
};

#endif
