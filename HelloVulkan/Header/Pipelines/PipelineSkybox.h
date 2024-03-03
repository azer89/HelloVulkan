#ifndef PIPELINE_SKYBOX
#define PIPELINE_SKYBOX

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "Configs.h"
#include "ResourcesShared.h"

#include <array>

class PipelineSkybox final : public PipelineBase
{
public:
	PipelineSkybox(VulkanContext& ctx, 
		VulkanImage* envMap,
		ResourcesShared* resShared,
		uint8_t renderBit = 0u
	);
	~PipelineSkybox();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	VulkanImage* envCubemap_;

	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;

	void CreateDescriptor(VulkanContext& ctx);
};

#endif
