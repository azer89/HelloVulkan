#ifndef PIPELINE_LIGHT
#define PIPELINE_LIGHT

#include "VulkanDevice.h"
#include "PipelineBase.h"
#include "Light.h"

/*
Render a light source, mostly for debugging purpose
*/
class PipelineLightRender final : public PipelineBase
{
public:
	PipelineLightRender(
		VulkanDevice& vkDev,
		Lights* lights,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u
	);
	~PipelineLightRender();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

	void RenderEnable(bool enable) { shouldRender_ = enable; }

private:
	void CreateDescriptor(VulkanDevice& vkDev);

private:
	Lights* lights_;
	std::vector<VkDescriptorSet> descriptorSets_;
	bool shouldRender_;
};

#endif