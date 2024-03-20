#ifndef RESOURCES_CLUSTER_FORWARD
#define RESOURCES_CLUSTER_FORWARD

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "Configs.h"

#include <array>

struct ResourcesClusterForward
{
public:
	ResourcesClusterForward() = default;
	~ResourcesClusterForward()
	{
		Destroy();
	}

	void Destroy();
	void CreateBuffers(VulkanContext& ctx, uint32_t lightCount);
	
public:
	bool aabbDirty_;
	VulkanBuffer aabbBuffer_;
	VulkanBuffer lightCellsBuffer_;
	VulkanBuffer lightIndicesBuffer_;

	// Use frame-in-flight because it is reset by the CPU
	std::array<VulkanBuffer, AppConfig::FrameCount> globalIndexCountBuffers_; // Atomic counter
};

#endif