#ifndef RENDERER_TONEMAP
#define RENDERER_TONEMAP

#include "RendererBase.h"
#include "VulkanImage.h"

class RendererTonemap final : public RendererBase
{
public:
	RendererTonemap(VulkanDevice& vkDev,
		VulkanImage* colorImage,
		VulkanImage* depthImage);
	~RendererTonemap() = default;

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	void CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);

private:
	VulkanImage* colorImage_;

	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif