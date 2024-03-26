#ifndef PIPELINE_SHADOW_MAPPING
#define PIPELINE_SHADOW_MAPPING

#include "PipelineBase.h"
#include "Scene.h"
#include "ResourcesShadow.h"
#include "BDA.h"

class PipelineShadow final : public PipelineBase
{
public:
	PipelineShadow(VulkanContext& ctx,
		Scene* scene,
		ResourcesShadow* resourcesShadow);
	~PipelineShadow();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void UpdateShadow(VulkanContext& ctx, ResourcesShadow* resShadow, glm::vec4 lightPosition);
	void OnWindowResized(VulkanContext& ctx) override;
	
private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	VIM vim_;
	Scene* scene_;
	ResourcesShadow* resourcesShadow_;

	std::vector<VulkanBuffer> shadowMapUBOBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;

	// Has its own buffers so that the buffers don't get culled
	VulkanBuffer indirectBuffer_;
};

#endif