#ifndef RENDERER_CUBE_FILTER
#define RENDERER_CUBE_FILTER

#include "RendererBase.h"
#include "VulkanTexture.h"

class RendererCubeFilter final : public RendererBase
{
public:
	RendererCubeFilter(VulkanDevice& vkDev, VulkanTexture* cubemapTexture);
	~RendererCubeFilter();

	void OffscreenRender(VulkanDevice& vkDev, VulkanTexture* cubemapTexture, VulkanTexture* irradianceTexture);
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VkFramebuffer frameBuffer_;
	VkSampler inputEnvMapSampler_; // A sampler for the input cubemapTexture

	void CreateRenderPass(VulkanDevice& vkDev);
	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, VulkanTexture* cubemapTexture);

	void InitializeIrradianceTexture(VulkanDevice& vkDev, VulkanTexture* irradianceTexture);
	void CreateCubemapViews(VulkanDevice& vkDev, 
		VulkanTexture* cubemapTexture, 
		std::vector<VkImageView>& cubemapViews);

	bool CreateOffsreenGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<const char*>& shaderFiles,
		VkPipeline* pipeline);

	VkFramebuffer CreateFrameBuffer(
		VulkanDevice& vkDev,
		std::vector<VkImageView> outputViews);
};

#endif