#ifndef PIPELINE_SSAO
#define PIPELINE_SSAO

#include "PipelineBase.h"
#include "ResourcesGBuffer.h"
#include "Scene.h"

class PipelineSSAO final : public PipelineBase
{
public:
	PipelineSSAO(VulkanContext& ctx,
		ResourcesGBuffer* resourcesGBuffer,
		uint8_t renderBit = 0u);
	~PipelineSSAO();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	void CreateDescriptor(VulkanContext& ctx);

	ResourcesGBuffer* resourcesGBuffer_;
	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif