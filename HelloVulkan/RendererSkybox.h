#ifndef RENDERER_CUBE
#define RENDERER_CUBE

#include "RendererBase.h"
#include "VulkanImage.h"

class RendererSkybox final : public RendererBase
{
public:
	RendererSkybox(VulkanDevice& vkDev, 
		VulkanImage* envMap,
		VulkanImage* depthImage,
		// If the last two parameters not provided, perform an offscreen render pass
		VulkanImage* offscreenColorImage = nullptr,
		uint8_t renderBit = 0u
	);
	~RendererSkybox();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanImage* specularMap_;

	std::vector<VkDescriptorSet> descriptorSets_;

	void CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);
};

#endif
