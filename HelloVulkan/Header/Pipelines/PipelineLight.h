#ifndef PIPELINE_LIGHT
#define PIPELINE_LIGHT

#include "VulkanDevice.h"
#include "PipelineBase.h"
#include "Light.h"

class PipelineLight final : public PipelineBase
{
public:
	PipelineLight(
		VulkanDevice& vkDev,
		Lights* lights,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u
	);
	~PipelineLight();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	void RenderEnable(bool enable) { shouldRender_ = enable; }

private:
	void CreateDescriptor(VulkanDevice& vkDev);

private:
	Lights* lights_;
	std::vector<VkDescriptorSet> descriptorSets_;
	bool shouldRender_;
};

#endif