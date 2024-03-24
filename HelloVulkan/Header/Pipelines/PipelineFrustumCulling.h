#ifndef PIPELINE_FRUSTUM_CULLING
#define PIPELINE_FRUSTUM_CULLING

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "Scene.h"
#include "Configs.h"

class PipelineFrustumCulling final : public PipelineBase
{
public:
	PipelineFrustumCulling(VulkanContext& ctx, Scene* scene);
	~PipelineFrustumCulling();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetFrustumUBO(VulkanContext& ctx, FrustumUBO& ubo)
	{
		const size_t frameIndex = ctx.GetFrameIndex();
		frustumBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(FrustumUBO));
	}

private:
	void Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex);

	void CreateDescriptor(VulkanContext& ctx);

private:
	Scene* scene_;
	std::vector<VulkanBuffer> frustumBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;
};

#endif