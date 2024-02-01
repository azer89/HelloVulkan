#ifndef RENDERER_CULL_LIGHTS
#define RENDERER_CULL_LIGHTS

#include "RendererBase.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"
#include "Light.h"
#include "ClusterForwardBuffers.h"
#include "UBO.h"

class RendererCullLights final : public RendererBase
{
public:
	RendererCullLights(VulkanDevice& vkDev, Lights* lights, ClusterForwardBuffers* cfBuffers);
	~RendererCullLights();

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	void FillComputeCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage);

	void ResetGlobalIndex(const VulkanDevice& vkDev, size_t currentImage)
	{
		uint32_t zeroValue = 0u;
		UpdateUniformBuffer(vkDev.GetDevice(), cfBuffers_->globalIndexCountBuffers_[currentImage], &zeroValue, sizeof(uint32_t));
	}

	void SetClusterForwardUBO(const VulkanDevice& vkDev, size_t currentImage, ClusterForwardUBO ubo)
	{
		UpdateUniformBuffer(vkDev.GetDevice(), cfUBOBuffers_[currentImage], &ubo, sizeof(ClusterForwardUBO));
	}

	void OnWindowResized(VulkanDevice& vkDev) override
	{
		// Not used
	}

private:
	Lights* lights_;
	ClusterForwardBuffers* cfBuffers_;
	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;
	VkPipeline pipeline_;

private:
	void CreateComputeDescriptorLayout(VulkanDevice& vkDev);

	void CreateComputeDescriptorSets(VulkanDevice& vkDev);

	void CreateComputePipeline(
		VulkanDevice& vkDev,
		VkShaderModule computeShader);
};

#endif