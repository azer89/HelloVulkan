#ifndef PIPELINE_PBR_SHADOW_MAPPING
#define PIPELINE_PBR_SHADOW_MAPPING

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "ResourcesIBL.h"
#include "Light.h"

#include <vector>

/*
Render meshes using PBR materials, naive forward renderer with shadow mapping
*/
class PipelinePBRShadow final : public PipelineBase
{
public:
	PipelinePBRShadow(VulkanContext& ctx,
		Scene* scene,
		Lights* lights,
		ResourcesIBL* iblResources,
		VulkanImage* shadowMap,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBRShadow();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };

	void SetShadowMapConfigUBO(VulkanContext& ctx, ShadowMapUBO ubo)
	{
		uint32_t frameIndex = ctx.GetFrameIndex();
		shadowMapConfigUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ShadowMapUBO));
	}

private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	PushConstPBR pc_;
	Lights* lights_;
	ResourcesIBL* iblResources_;
	VulkanImage* shadowMap_;
	Scene* scene_;
	std::vector<VkDescriptorSet> descriptorSets_;
	std::vector<VulkanBuffer> shadowMapConfigUBOBuffers_;
	std::vector<VulkanBuffer> indirectBuffers_;
};

#endif