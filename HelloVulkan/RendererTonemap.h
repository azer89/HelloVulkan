#ifndef RENDERER_TONEMAP
#define RENDERER_TONEMAP

#include "RendererBase.h"
#include "VulkanImage.h"

class RendererTonemap final : public RendererBase
{
public:
	RendererTonemap(VulkanDevice& vkDev,
		VulkanImage* singleSampledColorImage);
	~RendererTonemap() = default;

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	void OnWindowResized(VulkanDevice& vkDev) override;

private:
	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void AllocateDescriptorSets(VulkanDevice& vkDev);
	void UpdateDescriptorSets(VulkanDevice& vkDev);

private:
	VulkanImage* singleSampledColorImage_;

	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif