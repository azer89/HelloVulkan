#ifndef RENDERER_IRRADIANCE
#define RENDERER_IRRADIANCE

#include "RendererBase.h"
#include "VulkanTexture.h"

class RendererIrradiance final : public RendererBase
{
	RendererIrradiance(VulkanDevice& vkDev);
	~RendererIrradiance();

	void OfflineRender(VulkanDevice& vkDev, VulkanTexture* cubemapTexture, VulkanTexture* irradianceTexture);
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VkDescriptorSet descriptorSet_;
	VkFramebuffer frameBuffer_;

	void InitializeIrradianceTexture(VulkanDevice& vkDev, VulkanTexture* irradianceTexture);

};

#endif