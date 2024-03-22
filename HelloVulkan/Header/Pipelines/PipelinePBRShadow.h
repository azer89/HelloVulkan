#ifndef PIPELINE_PBR_SHADOW_MAPPING
#define PIPELINE_PBR_SHADOW_MAPPING

#include "PipelineBase.h"
#include "ResourcesLight.h"
#include "ResourcesShadow.h"
#include "ResourcesShared.h"
#include "ResourcesIBL.h"
#include "PushConstants.h"

#include <vector>

/*
Render meshes using PBR materials, naive forward renderer with shadow mapping
*/
class PipelinePBRShadow final : public PipelineBase
{
public:
	PipelinePBRShadow(VulkanContext& ctx,
		Scene* scene,
		ResourcesLight* resLight,
		ResourcesIBL* iblResources,
		ResourcesShadow* resShadow,
		ResourcesShared* resShared,
		uint8_t renderBit = 0u);
	 ~PipelinePBRShadow();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };

	void SetShadowMapConfigUBO(VulkanContext& ctx, ShadowMapUBO& ubo)
	{
		uint32_t frameIndex = ctx.GetFrameIndex();
		shadowMapConfigUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ShadowMapUBO));
	}

private:
	void PrepareVIM(VulkanContext& ctx);
	void CreateDescriptor(VulkanContext& ctx);
	void CreateSpecializationConstants();

private:
	PushConstPBR pc_;
	Scene* scene_;
	VulkanBuffer vimBuffer_;
	ResourcesLight* resLight_;
	ResourcesIBL* iblResources_;
	ResourcesShadow* resShadow_;
	std::vector<VkDescriptorSet> descriptorSets_;
	std::vector<VulkanBuffer> shadowMapConfigUBOBuffers_;

	// Specialization constants
	uint32_t alphaDiscard_;
	std::vector<VkSpecializationMapEntry> specializationEntries_;
};

#endif