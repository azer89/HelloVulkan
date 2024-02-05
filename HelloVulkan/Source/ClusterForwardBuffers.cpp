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
	// TODO Find a way to reduce duplicate
	uint32_t bufferCount = AppConfig::FrameOverlapCount;

	// AABB
	uint32_t aabbBufferSize = ClusterForwardConfig::numClusters * sizeof(AABB);
	aabbBuffers_.resize(bufferCount);
	aabbDirtyFlags_.resize(bufferCount);
	SetAABBDirty();

	// Global Index Count
	globalIndexCountBuffers_.resize(bufferCount);

	// LightCell
	uint32_t lightCellsBufferSize = ClusterForwardConfig::numClusters * sizeof(LightCell);
	lightCellsBuffers_.resize(bufferCount);

	// Light Indices
	uint32_t lightIndicesBufferSize =
		ClusterForwardConfig::maxLightPerCluster *
		ClusterForwardConfig::numClusters *
		sizeof(uint32_t);
	lightIndicesBuffers_.resize(bufferCount);

	for (uint32_t i = 0; i < bufferCount; ++i)
	{
		aabbBuffers_[i].CreateSharedBuffer(vkDev, aabbBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		globalIndexCountBuffers_[i].CreateSharedBuffer(vkDev, sizeof(uint32_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		lightCellsBuffers_[i].CreateSharedBuffer(vkDev, lightCellsBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		lightIndicesBuffers_[i].CreateSharedBuffer(vkDev, lightIndicesBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
}

void ClusterForwardBuffers::Destroy(VkDevice device)
{
	for (VulkanBuffer& buffer : aabbBuffers_)
	{
		buffer.Destroy(device);
	}

	for (VulkanBuffer& buffer : globalIndexCountBuffers_)
	{
		buffer.Destroy(device);
	}

	for (VulkanBuffer& buffer : lightCellsBuffers_)
	{
		buffer.Destroy(device);
	}

	for (VulkanBuffer& buffer : lightIndicesBuffers_)
	{
		buffer.Destroy(device);
	}
}