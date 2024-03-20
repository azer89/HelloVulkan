#include "ResourcesClusterForward.h"
#include "ResourcesLight.h"
#include "Configs.h"

void ResourcesClusterForward::CreateBuffers(VulkanContext& ctx, uint32_t lightCount)
{
	aabbDirty_ = true;
	constexpr uint32_t aabbBufferSize = ClusterForwardConfig::ClusterCount * sizeof(AABB);
	constexpr uint32_t lightCellsBufferSize = ClusterForwardConfig::ClusterCount * sizeof(LightCell);
	constexpr uint32_t lightIndicesBufferSize =
		ClusterForwardConfig::maxLightPerCluster *
		ClusterForwardConfig::ClusterCount *
		sizeof(uint32_t);

	aabbBuffer_.CreateBuffer(ctx, aabbBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0);
	lightCellsBuffer_.CreateBuffer(ctx, lightCellsBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0);
	lightIndicesBuffer_.CreateBuffer(ctx, lightIndicesBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0);

	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		globalIndexCountBuffers_[i].CreateBuffer(ctx, sizeof(uint32_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void ResourcesClusterForward::Destroy()
{
	aabbBuffer_.Destroy();
	lightCellsBuffer_.Destroy();
	lightIndicesBuffer_.Destroy();

	for (VulkanBuffer& buffer : globalIndexCountBuffers_)
	{
		buffer.Destroy();
	}
}