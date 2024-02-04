#ifndef CLUSTER_FORWARD_BUFFERS
#define CLUSTER_FORWARD_BUFFERS

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

#include <vector>

enum class AABBFlag : uint8_t
{
	Clean = 0u,
	Dirty = 1u
};

struct ClusterForwardBuffers
{
public:
	std::vector<AABBFlag> aabbDirtyFlags_;
	std::vector<VulkanBuffer> aabbBuffers_;
	std::vector<VulkanBuffer> globalIndexCountBuffers_;
	std::vector<VulkanBuffer> lightCellsBuffers_;
	std::vector<VulkanBuffer> lightIndicesBuffers_;

	void CreateBuffers(VulkanDevice& vkDev, uint32_t lightCount);
	void SetAABBDirty();
	bool IsAABBDirty(uint32_t frameIndex) { return aabbDirtyFlags_[frameIndex] == AABBFlag::Dirty; }
	void SetAABBClean(uint32_t frameIndex) { aabbDirtyFlags_[frameIndex] = AABBFlag::Clean; }
	void Destroy(VkDevice device);
};

#endif