#ifndef PIPELINE_TONEMAP
#define PIPELINE_TONEMAP

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "Configs.h"

#include <array>

/*
This applies tonemap to a color image and transfers it to a swapchain image
*/
class PipelineTonemap final : public PipelineBase
{
public:
	PipelineTonemap(VulkanDevice& vkDev,
		VulkanImage* singleSampledColorImage);
	~PipelineTonemap() = default;

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanDevice& vkDev) override;

private:
	void CreateDescriptor(VulkanDevice& vkDev);
	void AllocateDescriptorSets(VulkanDevice& vkDev);
	void UpdateDescriptorSets(VulkanDevice& vkDev);

private:
	VulkanImage* singleSampledColorImage_;

	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;
};

#endif