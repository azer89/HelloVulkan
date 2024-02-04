#ifndef PIPELINE_SKYBOX
#define PIPELINE_SKYBOX

#include "PipelineBase.h"
#include "VulkanImage.h"

class PipelineSkybox final : public PipelineBase
{
public:
	PipelineSkybox(VulkanDevice& vkDev, 
		VulkanImage* envMap,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u
	);
	~PipelineSkybox();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

private:
	VulkanImage* envCubemap_;

	std::vector<VkDescriptorSet> descriptorSets_;

	void CreateDescriptor(VulkanDevice& vkDev);
};

#endif
