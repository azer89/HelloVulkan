#ifndef CLUSTER_FORWARD_BUFFERS
#define CLUSTER_FORWARD_BUFFERS

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

#include <vector>

// TODO Rename to ClusterForwardResources
struct ClusterForwardBuffers
{
public:
	// TODO Make multiple buffers and dirty flags
	VulkanBuffer aabbBuffer_;

	// TODO Add ubo struct

	// TODO reduce the array light into AppConfig::FrameOverlapCount
	std::vector<VulkanBuffer> globalIndexCountBuffers_;
	std::vector<VulkanBuffer> lightCellsBuffers_;
	std::vector<VulkanBuffer> lightIndicesBuffers_;

	void CreateBuffers(VulkanDevice& vkDev, uint32_t lightCount);

	void Destroy(VkDevice device);
};

#endif