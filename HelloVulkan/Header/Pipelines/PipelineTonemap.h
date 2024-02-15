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
	PipelineTonemap(VulkanContext& vkDev,
		VulkanImage* singleSampledColorImage);
	~PipelineTonemap() = default;

	virtual void FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanContext& vkDev) override;

private:
	void CreateDescriptor(VulkanContext& vkDev);
	void AllocateDescriptorSets(VulkanContext& vkDev);
	void UpdateDescriptorSets(VulkanContext& vkDev);

private:
	VulkanImage* singleSampledColorImage_;

	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;
};

#endif