#ifndef RENDERER_EQUIRECT_2_CUBE
#define RENDERER_EQUIRECT_2_CUBE

#include "PipelineBase.h"
#include "VulkanImage.h"

#include <string>

class PipelineEquirect2Cube final : public PipelineBase
{
public:
	PipelineEquirect2Cube(VulkanDevice& vkDev, const std::string& hdrFile);
	~PipelineEquirect2Cube();

	void OffscreenRender(VulkanDevice& vkDev, VulkanImage* outputCubemap);

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VulkanImage inputHDRImage_;
	// TODO replace this below with framebuffer_
	VkFramebuffer cubeFramebuffer_;

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
