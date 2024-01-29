#ifndef PIPELINE_CUBE_FILTER
#define PIPELINE_CUBE_FILTER

#include "PipelineBase.h"
#include "VulkanImage.h"

#include <string>

enum class CubeFilterType : unsigned int
{
	// Iradiance / diffuse map
	Diffuse = 0,

	// Prefilter / specular map (mipmapped)
	Specular = 1, 
};

struct PushConstantCubeFilter
{
	float roughness = 0.f;
	uint32_t outputDiffuseSampleCount = 1u;
};

class PipelineCubeFilter final : public PipelineBase
{
public:
	PipelineCubeFilter(VulkanDevice& vkDev, VulkanImage* inputCubemap);
	~PipelineCubeFilter();

	void OffscreenRender(VulkanDevice& vkDev, 
		VulkanImage* outputCubemap,
		CubeFilterType filterType);

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

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

	// TODO Use VulkanFramebuffer
	VkFramebuffer CreateFrameBuffer(
		VulkanDevice& vkDev,
		std::vector<VkImageView> outputViews,
		uint32_t width,
		uint32_t height);
};

#endif