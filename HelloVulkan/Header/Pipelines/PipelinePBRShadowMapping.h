#ifndef PIPELINE_PBR_SHADOW_MAPPING
#define PIPELINE_PBR_SHADOW_MAPPING

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "Model.h"
#include "Light.h"

/*
Render meshes using PBR materials, naive forward renderer with shadow mapping
*/
class PipelinePBRShadowMapping final : public PipelineBase
{
public:
	PipelinePBRShadowMapping(VulkanContext& ctx,
		std::vector<Model*> models,
		Lights* lights,
		VulkanImage* specularMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		VulkanImage* shadowMap,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBRShadowMapping();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstantPBR& pbrPC) { pc_ = pbrPC; };

	void SetShadowMapConfigUBO(VulkanContext& ctx, ShadowMapConfigUBO ubo)
	{
		uint32_t frameIndex = ctx.GetFrameIndex();
		shadowMapConfigUBOBuffers_[frameIndex].UploadBufferData(ctx, 0, &ubo, sizeof(ShadowMapConfigUBO));
	}

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	void CreateDescriptor(VulkanContext& ctx);
	void CreateDescriptorSet(VulkanContext& ctx, Model* parentModel, Mesh& mesh);

private:
	std::vector<VulkanBuffer> shadowMapConfigUBOBuffers_;

	Lights* lights_;

	// Image-Based Lighting
	// TODO Organize these inside a struct
	VulkanImage* specularCubemap_;
	VulkanImage* diffuseCubemap_;
	VulkanImage* brdfLUT_;
	VulkanImage* shadowMap_;

	PushConstantPBR pc_;
};

#endif
