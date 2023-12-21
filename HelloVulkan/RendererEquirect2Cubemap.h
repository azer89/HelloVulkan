#ifndef RENDERER_EQUIRECT_2_CUBEMAP
#define RENDERER_EQUIRECT_2_CUBEMAP

#include "RendererBase.h"
#include "VulkanTexture.h"

#include "glm/glm.hpp"

#include <string>

class RendererEquirect2Cubemap : public RendererBase
{
public:
	RendererEquirect2Cubemap(VulkanDevice& vkDev, const std::string& hdrFile);
	~RendererEquirect2Cubemap();

	void OfflineRender(VulkanDevice& vkDev, VulkanTexture* cubemapTexture);
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VulkanTexture hdrTexture_;
	VkFramebuffer frameBuffer_;

private:
	void InitializeCubemapTexture(VulkanDevice& vkDev, VulkanTexture* cubemapTexture);
	void InitializeHDRTexture(VulkanDevice& vkDev, const std::string& hdrFile);
	void CreateRenderPass(VulkanDevice& vkDev);
	bool CreateDescriptorLayout(VulkanDevice& vkDev);
	bool CreateDescriptorSet(VulkanDevice& vkDev);

	bool CreateCustomGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<const char*>& shaderFiles,
		VkPipeline* pipeline);

	void CreateFrameBuffer(VulkanDevice& vkDev, std::vector<VkImageView> inputCubeMapViews);

	
};

#endif
