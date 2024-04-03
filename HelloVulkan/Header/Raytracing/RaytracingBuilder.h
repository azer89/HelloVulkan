#ifndef RAYTRACING_BUILDER
#define RAYTRACING_BUILDER

#include "VulkanContext.h"
#include "AccelStructure.h"
#include "VulkanBuffer.h"
#include "RTModelData.h"
#include "Scene.h"

#include <span>

class RaytracingBuilder
{
public:
	static void CreateRTModelData(VulkanContext& ctx,
		const std::span<VertexData> vertices,
		const std::span<uint32_t> indices,
		const glm::mat4 modelMatrix,
		RTModelData* modelData);

	static void CreateTransformBuffer(
		VulkanContext& ctx,
		const std::span<ModelUBO> uboArray,
		VulkanBuffer& transformBuffer);

	static void CreateBLASMultiMesh(
		VulkanContext& ctx, 
		const VulkanBuffer& transformBuffer,
		const Scene* scene, 
		AccelStructure* blas);

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
