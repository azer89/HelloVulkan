#ifndef PIPELINE_LIGHT
#define PIPELINE_LIGHT

#include "VulkanContext.h"
#include "PipelineBase.h"
#include "ResourcesLight.h"
#include "ResourcesShared.h"
#include "Configs.h"

/*
Render a light source for debugging purpose
*/
class PipelineLightRender final : public PipelineBase
{
public:
	PipelineLightRender(
		VulkanContext& ctx,
		ResourcesLight* resourcesLights,
		ResourcesShared* resourcesShared,
		uint8_t renderBit = 0u
	);
	~PipelineLightRender();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void UpdateFromInputContext(VulkanContext& ctx, InputContext& inputContext) override
	{
		shouldRender_ = inputContext.renderLights_;
	}

private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	ResourcesLight* resLight_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;
	bool shouldRender_;
};

#endif