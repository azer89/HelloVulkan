#ifndef PIPELINE_SHADOW_MAPPING
#define PIPELINE_SHADOW_MAPPING

#include "PipelineBase.h"
#include "Model.h"

class PipelineShadow final : public PipelineBase
{
public:
	PipelineShadow(VulkanContext& ctx,
		const std::vector<Model*>& models,
		VulkanImage* shadowMap);
	~PipelineShadow();

	void SetShadowMapUBO(VulkanContext& ctx, ShadowMapUBO& ubo)
	{
		const uint32_t frameIndex = ctx.GetFrameIndex();
		shadowMapUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ShadowMapUBO));
	}

	virtual void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanContext& ctx) override;
	
private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	std::vector<Model*> models_;
	VulkanImage* shadowMap_;

	std::vector<VulkanBuffer> shadowMapUBOBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif