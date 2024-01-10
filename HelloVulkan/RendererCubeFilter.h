#ifndef RENDERER_CUBE_FILTER
#define RENDERER_CUBE_FILTER

#include "RendererBase.h"
#include "VulkanImage.h"

#include <string>

enum class CubeFilterType : unsigned int
{
	Diffuse = 0,
	Specular = 1, 
};

struct PushConstantCubeFilter
{
	float roughness = 0.f;
	uint32_t sampleCount = 1u;
};

class RendererCubeFilter final : public RendererBase
{
public:
	RendererCubeFilter(VulkanDevice& vkDev, VulkanImage* inputCubemap);
	~RendererCubeFilter();

	void OffscreenRender(VulkanDevice& vkDev, 
		VulkanImage* outputCubemap,
		CubeFilterType filterType);

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:

	VkDescriptorSet descriptorSet_;
	VkSampler inputCubemapSampler_; // A sampler for the input cubemap

	// Two pipelines for each of diffuse and specular maps
	std::vector<VkPipeline> graphicsPipelines_;

	void CreateDescriptorLayout(VulkanDevice& vkDev);

	void CreateDescriptorSet(VulkanDevice& vkDev, VulkanImage* inputCubemap);

	void InitializeOutputCubemap(VulkanDevice& vkDev, 
		VulkanImage* outputDiffuseCubemap,
		uint32_t numMipmap,
		uint32_t sideLength);

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