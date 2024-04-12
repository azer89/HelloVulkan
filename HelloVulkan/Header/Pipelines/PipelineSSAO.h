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

	void SetRadiusAndBias(float radius, float bias)
	{
		radius_ = radius;
		bias_ = bias;
	}

	void UpdateFromUIData(VulkanContext& ctx, UIData& uiData) override
	{
		SetRadiusAndBias(uiData.ssaoRadius_, uiData.ssaoBias_);
	}

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	float radius_;
	float bias_;

	ResourcesGBuffer* resourcesGBuffer_;
	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif