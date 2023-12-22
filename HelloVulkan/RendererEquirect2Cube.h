#ifndef RENDERER_EQUIRECT_2_CUBE
#define RENDERER_EQUIRECT_2_CUBE

#include "RendererBase.h"
#include "VulkanTexture.h"

#include "glm/glm.hpp"

#include <string>

class RendererEquirect2Cube final : public RendererBase
{
public:
	RendererEquirect2Cube(VulkanDevice& vkDev, const std::string& hdrFile);
	~RendererEquirect2Cube();

	void OffscreenRender(VulkanDevice& vkDev, VulkanTexture* outputEnvMap);

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VulkanTexture inputHDRTexture_;
	VkFramebuffer frameBuffer_;

private:
	void InitializeEnvironmentMap(VulkanDevice& vkDev, VulkanTexture* outputEnvMap);
	void InitializeHDRTexture(VulkanDevice& vkDev, const std::string& hdrFile);
	void CreateCubemapViews(
		VulkanDevice& vkDev,
		VulkanTexture* cubemapTexture,
		std::vector<VkImageView>& cubeMapViews);

	void CreateRenderPass(VulkanDevice& vkDev);
	bool CreateDescriptorLayout(VulkanDevice& vkDev);
	bool CreateDescriptorSet(VulkanDevice& vkDev);

	bool CreateOffscreenGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<const char*>& shaderFiles,
		VkPipeline* pipeline);

	void CreateFrameBuffer(VulkanDevice& vkDev, std::vector<VkImageView> outputViews);
};

#endif
