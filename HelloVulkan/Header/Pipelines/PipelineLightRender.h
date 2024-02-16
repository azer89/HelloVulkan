#ifndef PIPELINE_LIGHT
#define PIPELINE_LIGHT

#include "VulkanContext.h"
#include "PipelineBase.h"
#include "Light.h"
#include "Configs.h"

/*
Render a light source for debugging purpose
*/
class PipelineLightRender final : public PipelineBase
{
public:
	PipelineLightRender(
		VulkanContext& ctx,
		Lights* lights,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u
	);
	~PipelineLightRender();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void RenderEnable(bool enable) { shouldRender_ = enable; }

private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	Lights* lights_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;
	bool shouldRender_;
};

#endif