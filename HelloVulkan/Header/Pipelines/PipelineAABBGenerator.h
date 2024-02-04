#ifndef PIPELINE_AABB_GENERATOR
#define PIPELINE_AABB_GENERATOR

#include "PipelineBase.h"
#include "VulkanDevice.h"
#include "ClusterForwardBuffers.h"
#include "Camera.h"

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
		const size_t currentImage = vkDev.GetCurrentSwapchainImageIndex();
		cfUBOBuffers_[currentImage].UploadBufferData(vkDev, 0, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	ClusterForwardBuffers* cfBuffers_;

	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;

private:
	void Execute(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);
	void CreateDescriptor(VulkanDevice& vkDev);
};

#endif