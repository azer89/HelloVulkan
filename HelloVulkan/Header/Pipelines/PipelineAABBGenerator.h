#ifndef PIPELINE_AABB_GENERATOR
#define PIPELINE_AABB_GENERATOR

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "ClusterForwardBuffers.h"
#include "Camera.h"
#include "Configs.h"

#include <array>

/*
Clustered Forward
*/
class PipelineAABBGenerator final : public PipelineBase
{
public:
	PipelineAABBGenerator(VulkanDevice& vkDev, ClusterForwardBuffers* cfBuffers);
	~PipelineAABBGenerator();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;
	void OnWindowResized(VulkanDevice& vkDev) override;

	void SetClusterForwardUBO(VulkanDevice& vkDev, ClusterForwardUBO ubo)
	{
		const size_t frameIndex = vkDev.GetFrameIndex();
		cfUBOBuffers_[frameIndex].UploadBufferData(vkDev, 0, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	ClusterForwardBuffers* cfBuffers_;

	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;

private:
	void Execute(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, uint32_t frameIndex);
	void CreateDescriptor(VulkanDevice& vkDev);
};

#endif