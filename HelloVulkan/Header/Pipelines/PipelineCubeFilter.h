#ifndef PIPELINE_CUBE_FILTER
#define PIPELINE_CUBE_FILTER

#include "PipelineBase.h"

#include <string>

class VulkanImage;

enum class CubeFilterType : uint8_t
{
	// Iradiance / diffuse map
	Diffuse = 0u,

	// Prefilter / specular map (mipmapped)
	Specular = 1u, 
};

/*
Offscreen pipeline to create specular map and diffuse map.
This class actually has two graphics pipelines.
*/
class PipelineCubeFilter final : public PipelineBase
{
public:
	PipelineCubeFilter(VulkanContext& ctx, VulkanImage* inputCubemap);
	~PipelineCubeFilter();

	void OffscreenRender(VulkanContext& ctx, 
		VulkanImage* outputCubemap,
		CubeFilterType filterType);

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:

	VkDescriptorSet descriptorSet_;
	VkSampler inputCubemapSampler_; // A sampler for the input cubemap

	// Two pipelines for each of diffuse and specular maps
	std::vector<VkPipeline> graphicsPipelines_;

	void CreateDescriptor(VulkanContext& ctx, VulkanImage* inputCubemap);

	void InitializeOutputCubemap(VulkanContext& ctx, 
		VulkanImage* outputDiffuseCubemap,
		uint32_t numMipmap,
		uint32_t inputCubeSideLength);

	void CreateOutputCubemapViews(VulkanContext& ctx,
		VulkanImage* outputCubemap,
		std::vector<std::vector<VkImageView>>& outputCubemapViews,
		uint32_t numMip);

	void CreateOffscreenGraphicsPipeline(
		VulkanContext& ctx,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		uint32_t viewportWidth,
		uint32_t viewportHeight,
		VkPipeline* pipeline);
};

#endif