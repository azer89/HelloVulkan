#ifndef PIPELINE_EQUIRECT_2_CUBE
#define PIPELINE_EQUIRECT_2_CUBE

#include "PipelineBase.h"
#include "VulkanImage.h"

#include <string>

/*
Offscreen pipeline to generate a cubemap from an HDR image
*/
class PipelineEquirect2Cube final : public PipelineBase
{
public:
	PipelineEquirect2Cube(VulkanContext& vkDev, const std::string& hdrFile);
	~PipelineEquirect2Cube();

	void OffscreenRender(VulkanContext& vkDev, VulkanImage* outputCubemap);

	void FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer) override;

private:
	VkDescriptorSet descriptorSet_;
	VulkanImage inputHDRImage_;
	// TODO replace this below with framebuffer_
	VkFramebuffer cubeFramebuffer_;

private:
	void InitializeHDRImage(VulkanContext& vkDev, const std::string& hdrFile);
	void InitializeCubemap(VulkanContext& vkDev, VulkanImage* cubemap);
	void CreateCubemapViews(
		VulkanContext& vkDev,
		VulkanImage* cubemap,
		std::vector<VkImageView>& cubemapViews);

	void CreateDescriptor(VulkanContext& vkDev);

	void CreateOffscreenGraphicsPipeline(
		VulkanContext& vkDev,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline);

	void CreateFrameBuffer(VulkanContext& vkDev, std::vector<VkImageView> outputViews);
};

#endif
