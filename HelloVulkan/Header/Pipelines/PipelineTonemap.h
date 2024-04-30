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
	PipelineTonemap(VulkanContext& ctx,
		VulkanImage* singleSampledColorImage);
	~PipelineTonemap() = default;

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	virtual void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanContext& ctx) override;

private:
	void CreateDescriptor(VulkanContext& ctx);
	void AllocateDescriptorSets(VulkanContext& ctx);
	void UpdateDescriptorSets(VulkanContext& ctx);

private:
	VulkanImage* singleSampledColorImage_;

	VulkanDescriptorSetInfo descriptorSetInfo_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;
};

#endif