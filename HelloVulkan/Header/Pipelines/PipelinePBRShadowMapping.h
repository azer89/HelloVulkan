#ifndef PIPELINE_PBR_SHADOW_MAPPING
#define PIPELINE_PBR_SHADOW_MAPPING

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "IBLResources.h"
#include "Model.h"
#include "Light.h"

#include <vector>

/*
Render meshes using PBR materials, naive forward renderer with shadow mapping
*/
class PipelinePBRShadowMapping final : public PipelineBase
{
public:
	PipelinePBRShadowMapping(VulkanContext& ctx,
		std::vector<Model*> models,
		Lights* lights,
		IBLResources* iblResources,
		VulkanImage* shadowMap,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBRShadowMapping();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstantPBR& pbrPC) { pc_ = pbrPC; };

	void SetShadowMapConfigUBO(VulkanContext& ctx, ShadowMapUBO ubo)
	{
		uint32_t frameIndex = ctx.GetFrameIndex();
		shadowMapConfigUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ShadowMapUBO));
	}

private:
	void CreateDescriptor(VulkanContext& ctx);
	void CreateDescriptorSet(VulkanContext& ctx, Model* parentModel, Mesh* mesh, const size_t meshIndex);

private:
	PushConstantPBR pc_;
	Lights* lights_;
	IBLResources* iblResources_;
	VulkanImage* shadowMap_;
	std::vector<Model*> models_;
	std::vector<std::vector<VkDescriptorSet>> descriptorSets_;
	std::vector<VulkanBuffer> shadowMapConfigUBOBuffers_;
};

#endif
