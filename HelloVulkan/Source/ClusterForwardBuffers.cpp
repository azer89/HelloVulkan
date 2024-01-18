#include "ClusterForwardBuffers.h"
#include "Light.h"
#include "Configs.h"

void ClusterForwardBuffers::CreateBuffers(
	VulkanDevice& vkDev, 
	uint32_t lightCount)
{
	uint32_t swapchainImageCount = vkDev.GetSwapchainImageCount();

	// AABB
	uint32_t aabbBufferSize = ClusterForwardConfig::numClusters * sizeof(AABB);
	aabbBuffer_.CreateSharedBuffer(vkDev, aabbBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Global Index Count
	globalIndexCountBuffers_.resize(swapchainImageCount);

	// LightCell
	uint32_t lightCellsBufferSize = ClusterForwardConfig::numClusters * sizeof(LightCell);
	lightCellsBuffers_.resize(swapchainImageCount);

	// Light Indices
	uint32_t lightIndicesBufferSize = 
		ClusterForwardConfig::maxLightPerCluster * 
		ClusterForwardConfig::numClusters *
		sizeof(uint32_t);
	lightIndicesBuffers_.resize(swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; ++i)
	{
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
	aabbBuffer_.Destroy(device);

	//globalIndexCountBuffer_.Destroy(device);
	//lightCellsBuffer_.Destroy(device);
	//lightIndicesBuffer_.Destroy(device);

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