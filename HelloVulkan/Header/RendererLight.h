#ifndef RENDERER_LIGHT
#define RENDERER_LIGHT

#include "VulkanDevice.h"
#include "RendererBase.h"
#include "Light.h"

class RendererLight final : public RendererBase
{
public:
	RendererLight(
		VulkanDevice& vkDev,
		Lights* lights,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u
	);
	~RendererLight();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	void RenderEnable(bool enable) { shouldRender_ = enable; }

private:
	void CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);

private:
	Lights* lights_;
	std::vector<VkDescriptorSet> descriptorSets_;
	bool shouldRender_;
};

#endif