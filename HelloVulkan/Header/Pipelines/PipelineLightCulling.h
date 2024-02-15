#ifndef PIPELINE_LIGHT_CULLING
#define PIPELINE_LIGHT_CULLING

#include "PipelineBase.h"
#include "VulkanDevice.h"
#include "ClusterForwardBuffers.h"
#include "Light.h"
#include "Configs.h"

#include <array>

/*
Clustered Forward
*/
class PipelineLightCulling final : public PipelineBase
{
public:
	PipelineLightCulling(VulkanDevice& vkDev, Lights* lights, ClusterForwardBuffers* cfBuffers);
	~PipelineLightCulling();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

	void ResetGlobalIndex(VulkanDevice& vkDev)
	{
		uint32_t zeroValue = 0u;
		uint32_t frameIndex = vkDev.GetFrameIndex();
		cfBuffers_->globalIndexCountBuffers_[frameIndex].UploadBufferData(vkDev, 0, &zeroValue, sizeof(uint32_t));
	}

	void SetClusterForwardUBO(VulkanDevice& vkDev, ClusterForwardUBO ubo)
	{
		size_t frameIndex = vkDev.GetFrameIndex();
		cfUBOBuffers_[frameIndex].UploadBufferData(vkDev, 0, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	Lights* lights_;
	ClusterForwardBuffers* cfBuffers_;

	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;

private:
	void Execute(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, uint32_t frameIndex);
	void CreateDescriptor(VulkanDevice& vkDev);
};

#endif