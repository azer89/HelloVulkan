#ifndef PIPELINE_PBR_CLUSTER_FORWARD
#define PIPELINE_PBR_CLUSTER_FORWARD

#include "PipelineBase.h"
#include "PushConstants.h"
#include "Model.h"

#include <vector>

struct ResourcesClusterForward;
struct ResourcesShared;
struct ResourcesLight;
struct ResourcesIBL;

/*
Render meshes using PBR materials, clustered forward renderer
*/
class PipelinePBRClusterForward final : public PipelineBase
{
public:
	PipelinePBRClusterForward(VulkanContext& ctx,
		std::vector<Model*> models,
		ResourcesLight* lights,
		ResourcesClusterForward* resCF,
		ResourcesIBL* iblResources,
		ResourcesShared* resShared,
		uint8_t renderBit = 0u);
	~PipelinePBRClusterForward();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };

	void SetClusterForwardUBO(VulkanContext& ctx, ClusterForwardUBO& ubo)
	{
		size_t frameIndex = ctx.GetFrameIndex();
		cfUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	PushConstPBR pc_;
	ResourcesClusterForward* resCF_;
	ResourcesLight* resLight_;
	ResourcesIBL* iblResources_;
	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::vector<Model*> models_;
	std::vector<std::vector<VkDescriptorSet>> descriptorSets_;

private:
	void CreateDescriptor(VulkanContext& ctx);
};

#endif