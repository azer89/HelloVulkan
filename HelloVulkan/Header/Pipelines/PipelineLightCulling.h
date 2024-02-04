#ifndef PIPELINE_LIGHT_CULLING
#define PIPELINE_LIGHT_CULLING

#include "PipelineBase.h"
#include "VulkanDevice.h"
#include "ClusterForwardBuffers.h"
#include "Light.h"

class PipelineLightCulling final : public PipelineBase
{
public:
	PipelineLightCulling(VulkanDevice& vkDev, Lights* lights, ClusterForwardBuffers* cfBuffers);
	~PipelineLightCulling();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

	void SetClusterForwardUBO(VulkanDevice& vkDev, ClusterForwardUBO ubo)
	{
		size_t currentImage = vkDev.GetCurrentSwapchainImageIndex();
		cfUBOBuffers_[currentImage].UploadBufferData(vkDev, 0, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	Lights* lights_;
	ClusterForwardBuffers* cfBuffers_;

	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;

private:
	void Execute(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);
	void CreateDescriptor(VulkanDevice& vkDev);
};

#endif