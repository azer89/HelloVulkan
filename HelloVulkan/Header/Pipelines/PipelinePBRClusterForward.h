#ifndef PIPELINE_PBR_CLUSTER_FORWARD
#define PIPELINE_PBR_CLUSTER_FORWARD

#include "PipelineBase.h"
#include "ResourcesClusterForward.h"
#include "PushConstants.h"
#include "ResourcesIBL.h"
#include "Model.h"
#include "Light.h"

#include <vector>

/*
Render meshes using PBR materials, clustered forward renderer
*/
class PipelinePBRClusterForward final : public PipelineBase
{
public:
	PipelinePBRClusterForward(VulkanContext& ctx,
		std::vector<Model*> models,
		Lights* lights,
		ResourcesClusterForward* resCF,
		ResourcesIBL* iblResources,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	~PipelinePBRClusterForward();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstantPBR& pbrPC) { pc_ = pbrPC; };

	void SetClusterForwardUBO(VulkanContext& ctx, ClusterForwardUBO ubo)
	{
		size_t frameIndex = ctx.GetFrameIndex();
		cfUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	PushConstantPBR pc_;
	ResourcesClusterForward* resCF_;
	Lights* lights_;
	ResourcesIBL* iblResources_;
	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::vector<Model*> models_;
	std::vector<std::vector<VkDescriptorSet>> descriptorSets_;

private:
	void CreateDescriptor(VulkanContext& ctx);
};

#endif