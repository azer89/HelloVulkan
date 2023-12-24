#ifndef RENDERER_CUBE_FILTER
#define RENDERER_CUBE_FILTER

#include "RendererBase.h"
#include "VulkanTexture.h"

#include <string>

enum class DistributionCubeFilter : unsigned int
{
	Lambertian = 0, // Diffuse
	GGX = 1, // Specular
};

struct PushConstantCubeFilter
{
	float roughness = 0.f;
	float lodBias = 0.f;
	uint32_t sampleCount = 1u;
	uint32_t mipLevel = 1u;
	uint32_t width = 1024u;
	DistributionCubeFilter distribution = DistributionCubeFilter::Lambertian;
};

class RendererCubeFilter final : public RendererBase
{
public:
	RendererCubeFilter(VulkanDevice& vkDev, VulkanTexture* inputCubemap);
	~RendererCubeFilter();

	void OffscreenRender(VulkanDevice& vkDev, 
		VulkanTexture* outputCubemap,
		DistributionCubeFilter disttribution);
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VkSampler inputEnvMapSampler_; // A sampler for the input cubemapTexture

	// Two pipelines for each of diffuse and specular maps
	std::vector<VkPipeline> graphicsPipelines_;

	void CreateRenderPass(VulkanDevice& vkDev);
	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, VulkanTexture* cubemapTexture);

	void InitializeOutputCubemap(VulkanDevice& vkDev, 
		VulkanTexture* outputDiffuseCubemap,
		uint32_t numMipmap,
		uint32_t sideLength);
	void CreateOutputCubemapViews(VulkanDevice& vkDev,
		VulkanTexture* cubemapTexture,
		std::vector<std::vector<VkImageView>>& cubemapViews,
		uint32_t numMip);

	void CreateOffsreenGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		uint32_t viewportWidth,
		uint32_t viewportHeight,
		VkPipeline* pipeline);

	VkFramebuffer CreateFrameBuffer(
		VulkanDevice& vkDev,
		std::vector<VkImageView> outputViews,
		uint32_t width,
		uint32_t height);
};

#endif