#ifndef RENDERER_AABB
#define RENDERER_AABB

#include "RendererBase.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"
#include "UBO.h"

class RendererAABB final : RendererBase
{
public:
	RendererAABB(VulkanDevice& vkDev);
	~RendererAABB();

	void CreateClusters(VulkanDevice& vkDev, const ClusterForwardUBO& ubo, VulkanBuffer* aabbBuffer);

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanBuffer cfUBOBuffer_;
	VkDescriptorSet computeDescriptorSet_;
	VkPipeline pipeline_;

private:
	void Execute(VulkanDevice& vkDev, VulkanBuffer* aabbBuffer);

	void CreateComputeDescriptorSetLayout(VkDevice device);

	void CreateComputeDescriptorSet(VkDevice device, VulkanBuffer* aabbBuffer);

	void CreateComputePipeline(
		VkDevice device,
		VkShaderModule computeShader);
};

#endif