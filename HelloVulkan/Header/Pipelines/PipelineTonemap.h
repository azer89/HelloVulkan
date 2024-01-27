#ifndef RENDERER_TONEMAP
#define RENDERER_TONEMAP

#include "PipelineBase.h"
#include "VulkanImage.h"

/*
This applies tonemap to a color image and transfers image to a swapchain image
*/
class PipelineTonemap final : public PipelineBase
{
public:
	PipelineTonemap(VulkanDevice& vkDev,
		VulkanImage* singleSampledColorImage);
	~PipelineTonemap() = default;

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