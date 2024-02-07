#include "ClusterForwardBuffers.h"
#include "Light.h"
#include "Configs.h"

// If window is resized, set the AABB buffers dirty
void ClusterForwardBuffers::SetAABBDirty()
{
	for (size_t i = 0; i < aabbDirtyFlags_.size(); ++i)
	{
		aabbDirtyFlags_[i] = AABBFlag::Dirty;
	}
}

void ClusterForwardBuffers::CreateBuffers(VulkanDevice& vkDev, uint32_t lightCount)
{
	constexpr uint32_t bufferCount = AppConfig::FrameOverlapCount;

	// AABB
	constexpr uint32_t aabbBufferSize = ClusterForwardConfig::numClusters * sizeof(AABB);
	aabbBuffers_.resize(bufferCount);
	aabbDirtyFlags_.resize(bufferCount);
	SetAABBDirty();

	// Global Index Count
	globalIndexCountBuffers_.resize(bufferCount);

	// LightCell
	constexpr uint32_t lightCellsBufferSize = ClusterForwardConfig::numClusters * sizeof(LightCell);
	lightCellsBuffers_.resize(bufferCount);

	// Light Indices
	constexpr uint32_t lightIndicesBufferSize =
		ClusterForwardConfig::maxLightPerCluster *
		ClusterForwardConfig::numClusters *
		sizeof(uint32_t);
	lightIndicesBuffers_.resize(bufferCount);

	for (uint32_t i = 0; i < bufferCount; ++i)
	{
		aabbBuffers_[i].CreateBuffer(vkDev, aabbBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY,
			0);

		globalIndexCountBuffers_[i].CreateBuffer(vkDev, sizeof(uint32_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);

		lightCellsBuffers_[i].CreateBuffer(vkDev, lightCellsBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY,
			0);

		lightIndicesBuffers_[i].CreateBuffer(vkDev, lightIndicesBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY,
			0);
	}
}

void ClusterForwardBuffers::Destroy(VkDevice device)
{
	for (VulkanBuffer& buffer : aabbBuffers_)
	{
		buffer.Destroy();
	}

	for (VulkanBuffer& buffer : globalIndexCountBuffers_)
	{
		buffer.Destroy();
	}

	for (VulkanBuffer& buffer : lightCellsBuffers_)
	{
		buffer.Destroy();
	}

	for (VulkanBuffer& buffer : lightIndicesBuffers_)
	{
		buffer.Destroy();
	}
}