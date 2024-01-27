#ifndef RENDERER_SKYBOX
#define RENDERER_SKYBOX

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

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanImage* envCubemap_;

	std::vector<VkDescriptorSet> descriptorSets_;

	void CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);
};

#endif
