#ifndef PIPELINE_SHADOW_MAPPING
#define PIPELINE_SHADOW_MAPPING

#include "PipelineBase.h"
#include "ResourcesShadow.h"
#include "ResourcesLight.h"
#include "Camera.h"
#include "Scene.h"
#include "Configs.h"

#include <array>

class PipelineShadow final : public PipelineBase
{
public:
	PipelineShadow(VulkanContext& ctx,
		Scene* scene,
		ResourcesShadow* resShadow);
	~PipelineShadow();

	void CalculateCascade(VulkanContext& ctx, 
		const Camera* camera,
		const LightData* light, 
		ShadowMapUBO* ubo);

	virtual void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanContext& ctx) override;
	
private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	Scene* scene_;
	ResourcesShadow* resShadow_;

	std::array<VulkanFramebuffer, ShadowConfig::CascadeCount> cascadeFramebuffers_;

	std::vector<VulkanBuffer> shadowMapUBOBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;
	std::vector<VulkanBuffer> indirectBuffers_;
};

#endif