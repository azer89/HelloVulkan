#ifndef CLUSTER_FORWARD_BUFFERS
#define CLUSTER_FORWARD_BUFFERS

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

#include <vector>

struct ClusterForwardBuffers
{
public:
	std::vector<unsigned int> aabbDirtyFlags_;
	std::vector<VulkanBuffer> aabbBuffers_;
	std::vector<VulkanBuffer> globalIndexCountBuffers_;
	std::vector<VulkanBuffer> lightCellsBuffers_;
	std::vector<VulkanBuffer> lightIndicesBuffers_;

	void CreateBuffers(VulkanDevice& vkDev, uint32_t lightCount);

	void Destroy(VkDevice device);
};

#endif