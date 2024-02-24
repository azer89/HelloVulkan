#ifndef PIPELINE_PBR_BINDLESS
#define PIPELINE_PBR_BINDLESS

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "IBLResources.h"
#include "Scene.h"
#include "Light.h"

#include <vector>

/*
Render a scene using PBR materials, naive forward renderer, bindless
*/
class PipelinePBRBindless final : public PipelineBase
{
public:
	PipelinePBRBindless(VulkanContext& ctx,
		Scene* scene,
		Lights* lights,
		IBLResources* iblResources,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBRBindless();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void SetPBRPushConstants(const PushConstantPBR& pbrPC) { pc_ = pbrPC; };

private:
	void CreateIndirectBuffers(VulkanContext& ctx);
	void CreateDescriptor(VulkanContext& ctx);

	Scene* scene_;
	Lights* lights_;
	IBLResources* iblResources_;
	PushConstantPBR pc_;
	std::vector<VulkanBuffer> indirectBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif