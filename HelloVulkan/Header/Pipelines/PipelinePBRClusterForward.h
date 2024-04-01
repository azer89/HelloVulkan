#ifndef PIPELINE_PBR_CLUSTER_FORWARD
#define PIPELINE_PBR_CLUSTER_FORWARD

#include "PipelineBase.h"
#include "ResourcesClusterForward.h"
#include "ResourcesShared.h"
#include "ResourcesLight.h"
#include "ResourcesIBL.h"
#include "PushConstants.h"
#include "Scene.h"

#include <vector>

/*
Render meshes using PBR materials, clustered forward renderer
*/
class PipelinePBRClusterForward final : public PipelineBase
{
public:
	PipelinePBRClusterForward(VulkanContext& ctx,
		Scene* scene,
		ResourcesLight* lights,
		ResourcesClusterForward* resourcesCF,
		ResourcesIBL* resourcesIBL,
		ResourcesShared* resourcesShared,
		MaterialType materialType,
		uint8_t renderBit = 0u);
	~PipelinePBRClusterForward();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };

	void SetClusterForwardUBO(VulkanContext& ctx, ClusterForwardUBO& ubo)
	{
		const size_t frameIndex = ctx.GetFrameIndex();
		cfUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ClusterForwardUBO));
	}

	void UpdateFromIUData(VulkanContext& ctx, UIData& uiData) override
	{
		SetPBRPushConstants(uiData.pbrPC_);
	}

private:
	void PrepareBDA(VulkanContext& ctx);
	void CreateDescriptor(VulkanContext& ctx);
	void CreateSpecializationConstants();

private:
	PushConstPBR pc_;
	ResourcesClusterForward* resourcesCF_;
	ResourcesLight* resourcesLight_;
	ResourcesIBL* resourcesIBL_;
	std::vector<VulkanBuffer> cfUBOBuffers_;
	VulkanBuffer bdaBuffer_;
	Scene* scene_;
	std::vector<VkDescriptorSet> descriptorSets_;

	// Material pass
	MaterialType materialType_;
	VkDeviceSize materialOffset_;
	uint32_t materialDrawCount_;

	// Specialization constants
	uint32_t alphaDiscard_;

};

#endif