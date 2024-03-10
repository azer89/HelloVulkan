#ifndef PIPELINE_SHADOW_MAPPING
#define PIPELINE_SHADOW_MAPPING

#include "PipelineBase.h"
#include "VIM.h"

class Scene;
struct ResourcesShadow;

class PipelineShadow final : public PipelineBase
{
public:
	PipelineShadow(VulkanContext& ctx,
		Scene* scene,
		ResourcesShadow* resShadow);
	~PipelineShadow();

	void UpdateShadow(VulkanContext& ctx, ResourcesShadow* resShadow, glm::vec4 lightPosition);

	virtual void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanContext& ctx) override;
	
private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	VIM vim_;
	Scene* scene_;
	ResourcesShadow* resShadow_;

	std::vector<VulkanBuffer> shadowMapUBOBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;
	std::vector<VulkanBuffer> indirectBuffers_;
};

#endif