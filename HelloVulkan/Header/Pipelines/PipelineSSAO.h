#ifndef PIPELINE_SSAO
#define PIPELINE_SSAO

#include "PipelineBase.h"
#include "ResourcesGBuffer.h"
#include "Scene.h"
#include "UBOs.h"

#include <array>

class PipelineSSAO final : public PipelineBase
{
public:
	PipelineSSAO(VulkanContext& ctx,
		ResourcesGBuffer* resourcesGBuffer,
		uint8_t renderBit = 0u);
	~PipelineSSAO();

	void SetParameters(float radius, float bias, float power)
	{
		radius_ = radius;
		bias_ = bias;
		power_ = power;
	}

	void UpdateFromUIData(VulkanContext& ctx, UIData& uiData) override
	{
		SetParameters(uiData.ssaoRadius_, uiData.ssaoBias_, uiData.ssaoPower_);
	}

	void OnWindowResized(VulkanContext& ctx) override;

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override 
	{
		SSAOUBO ssaoUbo =
		{
			.projection = ubo.projection,
			.radius = radius_,
			.bias = bias_,
			.power = power_,
			.screenWidth = static_cast<float>(ctx.GetSwapchainWidth()),
			.screenHeight = static_cast<float>(ctx.GetSwapchainHeight()),
			.noiseSize = resourcesGBuffer_->GetNoiseDimension()
		};
		const uint32_t frameIndex = ctx.GetFrameIndex();
		ssaoUboBuffers_[frameIndex].UploadBufferData(ctx, &ssaoUbo, sizeof(SSAOUBO));
	}

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	void CreateDescriptor(VulkanContext& ctx);
	void AllocateDescriptorSets(VulkanContext& ctx);
	void UpdateDescriptorSets(VulkanContext& ctx);

private:
	float radius_ = 0.0f;
	float bias_ = 0.0f;
	float power_ = 0.0f;

	ResourcesGBuffer* resourcesGBuffer_ = nullptr;
	std::vector<VulkanBuffer> ssaoUboBuffers_ = {};
	VulkanDescriptorInfo descriptorInfo_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_ = {};
};

#endif