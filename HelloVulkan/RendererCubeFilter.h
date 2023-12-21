#ifndef RENDERER_CUBE_FILTER
#define RENDERER_CUBE_FILTER

#include "RendererBase.h"
#include "VulkanTexture.h"

class RendererCubeFilter final : public RendererBase
{
	RendererCubeFilter(VulkanDevice& vkDev);
	~RendererCubeFilter();

	void OfflineRender(VulkanDevice& vkDev, VulkanTexture* cubemapTexture, VulkanTexture* irradianceTexture);
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VkFramebuffer frameBuffer_;

	void CreateRenderPass(VulkanDevice& vkDev);
	void InitializeIrradianceTexture(VulkanDevice& vkDev, VulkanTexture* irradianceTexture);
	void CreateCubemapViews(VulkanDevice& vkDev, 
		VulkanTexture* cubemapTexture, 
		std::vector<VkImageView>& cubemapViews);
};

#endif