#ifndef PIPELINE_SKYBOX
#define PIPELINE_SKYBOX

#include "PipelineBase.h"
#include "Configs.h"

#include <array>

struct ResourcesShared;
class VulkanImage;

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

	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;

	void CreateDescriptor(VulkanContext& ctx);
};

#endif
