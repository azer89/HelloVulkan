#ifndef PIPELINE_CUBE_FILTER
#define PIPELINE_CUBE_FILTER

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"

#include <string>

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
	PipelineCubeFilter(VulkanDevice& vkDev, VulkanImage* inputCubemap);
	~PipelineCubeFilter();

	void OffscreenRender(VulkanDevice& vkDev, 
		VulkanImage* outputCubemap,
		CubeFilterType filterType);

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

private:

	VkDescriptorSet descriptorSet_;
	VkSampler inputCubemapSampler_; // A sampler for the input cubemap

	// Two pipelines for each of diffuse and specular maps
	std::vector<VkPipeline> graphicsPipelines_;

	void CreateDescriptor(VulkanDevice& vkDev, VulkanImage* inputCubemap);

	void InitializeOutputCubemap(VulkanDevice& vkDev, 
		VulkanImage* outputDiffuseCubemap,
		uint32_t numMipmap,
		uint32_t inputCubeSideLength);

	void CreateOutputCubemapViews(VulkanDevice& vkDev,
		VulkanImage* outputCubemap,
		std::vector<std::vector<VkImageView>>& outputCubemapViews,
		uint32_t numMip);

	void CreateOffsreenGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		uint32_t viewportWidth,
		uint32_t viewportHeight,
		VkPipeline* pipeline);
};

#endif