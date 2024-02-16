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
	PipelineEquirect2Cube(VulkanContext& ctx, const std::string& hdrFile);
	~PipelineEquirect2Cube();

	void OffscreenRender(VulkanContext& ctx, VulkanImage* outputCubemap);

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	VkDescriptorSet descriptorSet_;
	VulkanImage inputHDRImage_;
	// TODO replace this below with framebuffer_
	VkFramebuffer cubeFramebuffer_;

private:
	void InitializeHDRImage(VulkanContext& ctx, const std::string& hdrFile);
	void InitializeCubemap(VulkanContext& ctx, VulkanImage* cubemap);
	void CreateCubemapViews(
		VulkanContext& ctx,
		VulkanImage* cubemap,
		std::vector<VkImageView>& cubemapViews);

	void CreateDescriptor(VulkanContext& ctx);

	void CreateOffscreenGraphicsPipeline(
		VulkanContext& ctx,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline);

	void CreateFrameBuffer(VulkanContext& ctx, std::vector<VkImageView> outputViews);
};

#endif
