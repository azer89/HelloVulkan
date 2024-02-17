#ifndef PIPELINE_SHADOW_MAPPING
#define PIPELINE_SHADOW_MAPPING

#include "PipelineBase.h"
#include "PushConstants.h"
#include "Model.h"

#include <array>

class PipelineShadow final : public PipelineBase
{
public:
	PipelineShadow(VulkanContext& ctx,
		std::vector<Model*> models,
		VulkanImage* shadowMap);
	~PipelineShadow();

	void SetShadowMapUBO(VulkanContext& ctx, glm::vec4 lightPosition, glm::vec4 lightTarget)
	{
		uint32_t frameIndex = ctx.GetFrameIndex();
		// TODO
	}

	virtual void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	
private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	std::vector<Model*> models_;
	VulkanImage* shadowMap_;

	std::vector<VulkanBuffer> shadowMapUBOBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif