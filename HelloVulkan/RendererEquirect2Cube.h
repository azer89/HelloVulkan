#ifndef RENDERER_EQUIRECT_2_CUBE
#define RENDERER_EQUIRECT_2_CUBE

#include "RendererBase.h"
#include "VulkanImage.h"

#include <string>

class RendererEquirect2Cube final : public RendererBase
{
public:
	RendererEquirect2Cube(VulkanDevice& vkDev, const std::string& hdrFile);
	~RendererEquirect2Cube();

	void OffscreenRender(VulkanDevice& vkDev, VulkanImage* outputCubemap);

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VulkanImage inputHDRImage_;
	VkFramebuffer frameBuffer_;

private:
	void InitializeHDRImage(VulkanDevice& vkDev, const std::string& hdrFile);
	void InitializeCubemap(VulkanDevice& vkDev, VulkanImage* cubemap);
	void CreateCubemapViews(
		VulkanDevice& vkDev,
		VulkanImage* cubemap,
		std::vector<VkImageView>& cubemapViews);

	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev);

	void CreateOffscreenGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline);

	void CreateFrameBuffer(VulkanDevice& vkDev, std::vector<VkImageView> outputViews);
};

#endif
