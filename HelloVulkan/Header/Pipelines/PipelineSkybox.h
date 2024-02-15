#ifndef PIPELINE_SKYBOX
#define PIPELINE_SKYBOX

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "Configs.h"

#include <array>

class PipelineSkybox final : public PipelineBase
{
public:
	PipelineSkybox(VulkanContext& vkDev, 
		VulkanImage* envMap,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u
	);
	~PipelineSkybox();

	void FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer) override;

private:
	VulkanImage* envCubemap_;

	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;

	void CreateDescriptor(VulkanContext& vkDev);
};

#endif
