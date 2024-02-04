#ifndef PIPELINE_AABB_GENERATOR
#define PIPELINE_AABB_GENERATOR

#include "PipelineBase.h"
#include "VulkanDevice.h"
#include "ClusterForwardBuffers.h"
#include "Camera.h"

class PipelineAABBGenerator final : public PipelineBase
{
public:
	PipelineAABBGenerator(VulkanDevice& vkDev, ClusterForwardBuffers* cfBuffers, Camera* camera);
	~PipelineAABBGenerator();

private:
	ClusterForwardBuffers* cfBuffers_;
	Camera* camera_;

	VulkanBuffer cfUBOBuffer_;
	VkDescriptorSet computeDescriptorSet_;
	VkPipeline pipeline_;

private:
	void Execute(VulkanDevice& vkDev, VkCommandBuffer commandBuffer);

	void CreateDescriptorPool(VkDevice device);
	void CreateDescriptorLayout(VkDevice device);

	void AllocateDescriptorSet(VkDevice device);
	void UpdateDescriptorSet(VkDevice device, VulkanBuffer* aabbBuffer);

	void CreateComputePipeline(
		VkDevice device,
		VkShaderModule computeShader);
};

#endif