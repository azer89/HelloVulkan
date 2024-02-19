#ifndef PIPELINE_PBR_CLUSTER_FORWARD
#define PIPELINE_PBR_CLUSTER_FORWARD

#include "PipelinePBR.h"
#include "ClusterForwardBuffers.h"

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
		ClusterForwardBuffers* cfBuffers,
		VulkanImage* specularMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
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
	ClusterForwardBuffers* cfBuffers_;
	std::vector<VulkanBuffer> cfUBOBuffers_;

	Lights* lights_;

	// Image-Based Lighting
	// TODO Organize these inside a struct
	VulkanImage* specularCubemap_;
	VulkanImage* diffuseCubemap_;
	VulkanImage* brdfLUT_;

	PushConstantPBR pc_;

	std::vector<Model*> models_;
	std::vector<std::vector<VkDescriptorSet>> descriptorSets_;

private:
	void CreateDescriptor(VulkanContext& ctx);
	void CreateDescriptorSet(VulkanContext& ctx, Model* parentModel, Mesh* mesh, const size_t meshIndex);
};

#endif