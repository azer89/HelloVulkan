#ifndef PIPELINE_TONEMAP
#define PIPELINE_TONEMAP

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "Configs.h"

#include <array>

/*
This applies tonemap to a color image and transfers it to a swapchain image.
*/
class PipelineTonemap final : public PipelineBase
{
public:
	PipelineTonemap(VulkanContext& ctx,
		VulkanImage* singleSampledColorImage);
	~PipelineTonemap() = default;

	virtual void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanContext& ctx) override;

private:
	void CreateDescriptor(VulkanContext& ctx);
	void AllocateDescriptorSets(VulkanContext& ctx);
	void UpdateDescriptorSets(VulkanContext& ctx);

private:
	VulkanImage* singleSampledColorImage_;

	DescriptorBuildInfo descriptorBuildInfo_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;
};

#endif