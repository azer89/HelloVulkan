#ifndef PIPELINE_PBR_SHADOW_MAPPING
#define PIPELINE_PBR_SHADOW_MAPPING

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "IBLResources.h"
#include "Model.h"
#include "Light.h"

#include <vector>

struct ShadowSpecializationConstants
{
	alignas(4)
	uint32_t pcfIteration = 2u;
};

/*
Render meshes using PBR materials, naive forward renderer with shadow mapping
*/
class PipelinePBRShadowMapping final : public PipelineBase
{
public:
	PipelinePBRShadowMapping(VulkanContext& ctx,
		Scene* scene,
		Lights* lights,
		IBLResources* iblResources,
		VulkanImage* shadowMap,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBRShadowMapping();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPCFIteration(uint32_t iter) { shadowSC_.pcfIteration = iter; }
	void SetPBRPushConstants(const PushConstantPBR& pbrPC) { pc_ = pbrPC; };
	void SetShadowMapConfigUBO(VulkanContext& ctx, ShadowMapUBO ubo)
	{
		uint32_t frameIndex = ctx.GetFrameIndex();
		shadowMapConfigUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ShadowMapUBO));
	}

private:
	void CreateSpecializationConstants();
	void CreateDescriptor(VulkanContext& ctx);

private:
	PushConstantPBR pc_;
	Lights* lights_;
	IBLResources* iblResources_;
	VulkanImage* shadowMap_;
	Scene* scene_;
	std::vector<VkDescriptorSet> descriptorSets_;
	std::vector<VulkanBuffer> shadowMapConfigUBOBuffers_;
	std::vector<VulkanBuffer> indirectBuffers_;

	ShadowSpecializationConstants shadowSC_;
	std::vector<VkSpecializationMapEntry> specializationEntries_;
};

#endif