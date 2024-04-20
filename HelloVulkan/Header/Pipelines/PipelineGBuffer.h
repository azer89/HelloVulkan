#ifndef PIPELINE_G_BUFFER
#define PIPELINE_G_BUFFER

#include "PipelineBase.h"
#include "ResourcesGBuffer.h"
#include "Scene.h"

class PipelineGBuffer final : public PipelineBase
{
public:
	PipelineGBuffer(VulkanContext& ctx,
		Scene* scene,
		ResourcesGBuffer* resourcesGBuffer,
		uint8_t renderBit = 0u);
	~PipelineGBuffer();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	void CreateBDABuffer(VulkanContext& ctx);
	void CreateDescriptor(VulkanContext& ctx);

	Scene* scene_;
	ResourcesGBuffer* resourcesGBuffer_;
	VulkanBuffer bdaBuffer_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;
};

#endif