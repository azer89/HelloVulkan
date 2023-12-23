#ifndef RENDERER_CUBE_FILTER
#define RENDERER_CUBE_FILTER

#include "RendererBase.h"
#include "VulkanTexture.h"

#include <string>

class RendererCubeFilter final : public RendererBase
{
public:
	RendererCubeFilter(VulkanDevice& vkDev, VulkanTexture* inputCubemap);
	~RendererCubeFilter();

	void OffscreenRender(VulkanDevice& vkDev, 
		VulkanTexture* inputCubemap, 
		VulkanTexture* outputDiffuseCubemap);
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VkFramebuffer frameBuffer_;
	VkSampler inputEnvMapSampler_; // A sampler for the input cubemapTexture

	void CreateRenderPass(VulkanDevice& vkDev);
	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, VulkanTexture* cubemapTexture);

	void InitializeDiffuseCubemap(VulkanDevice& vkDev, 
		VulkanTexture* outputDiffuseCubemap);
	void CreateCubemapViews(VulkanDevice& vkDev, 
		VulkanTexture* cubemapTexture, 
		std::vector<VkImageView>& cubemapViews);

	void CreateOffsreenGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline);

	VkFramebuffer CreateFrameBuffer(
		VulkanDevice& vkDev,
		std::vector<VkImageView> outputViews);
};

#endif