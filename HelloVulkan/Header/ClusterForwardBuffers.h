#ifndef CLUSTER_FORWARD_BUFFERS
#define CLUSTER_FORWARD_BUFFERS

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "Configs.h"

#include <array>

enum class AABBFlag : uint8_t
{
	Clean = 0u,
	Dirty = 1u
};

struct ClusterForwardBuffers
{
public:
	std::array<AABBFlag, AppConfig::FrameOverlapCount> aabbDirtyFlags_;
	std::array<VulkanBuffer, AppConfig::FrameOverlapCount> aabbBuffers_;
	std::array<VulkanBuffer, AppConfig::FrameOverlapCount> globalIndexCountBuffers_;
	std::array<VulkanBuffer, AppConfig::FrameOverlapCount> lightCellsBuffers_;
	std::array<VulkanBuffer, AppConfig::FrameOverlapCount> lightIndicesBuffers_;

	void CreateBuffers(VulkanContext& vkDev, uint32_t lightCount);
	void SetAABBDirty();
	bool IsAABBDirty(uint32_t frameIndex) { return aabbDirtyFlags_[frameIndex] == AABBFlag::Dirty; }
	void SetAABBClean(uint32_t frameIndex) { aabbDirtyFlags_[frameIndex] = AABBFlag::Clean; }
	void Destroy();
};

#endif