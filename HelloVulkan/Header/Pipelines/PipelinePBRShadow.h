#ifndef PIPELINE_PBR_SHADOW_MAPPING
#define PIPELINE_PBR_SHADOW_MAPPING

#include "PipelineBase.h"
#include "ResourcesLight.h"
#include "ResourcesShadow.h"
#include "ResourcesShared.h"
#include "ResourcesGBuffer.h"
#include "ResourcesIBL.h"
#include "Scene.h"
#include "PushConstants.h"

#include <vector>

/*
Render meshes using PBR materials, naive forward renderer with shadow mapping
*/
class PipelinePBRShadow final : public PipelineBase
{
public:
	PipelinePBRShadow(VulkanContext& ctx,
		Scene* scene,
		ResourcesLight* resourcesLight,
		ResourcesIBL* resourcesIBL,
		ResourcesShadow* resourcesShadow,
		ResourcesShared* resourcesShared,
		ResourcesGBuffer* resourcesGBuffer,
		MaterialType materialType,
		uint8_t renderBit = 0u);
	 ~PipelinePBRShadow();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void OnWindowResized(VulkanContext& ctx) override;
	void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };
	
	void SetShadowMapConfigUBO(VulkanContext& ctx, ShadowMapUBO& ubo)
	{
		const uint32_t frameIndex = ctx.GetFrameIndex();
		shadowMapConfigUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ShadowMapUBO));
	}
	
	void UpdateFromUIData(VulkanContext& ctx, UIData& uiData) override
	{
		SetPBRPushConstants(uiData.pbrPC_);
	}

private:
	void CreateBDABuffer(VulkanContext& ctx);
	void CreateSpecializationConstants();
	void CreateDescriptor(VulkanContext& ctx);
	void AllocateDescriptorSets(VulkanContext& ctx);
	void UpdateDescriptorSets(VulkanContext& ctx);

private:
	PushConstPBR pc_;
	Scene* scene_;
	VulkanBuffer bdaBuffer_;
	ResourcesLight* resourcesLight_;
	ResourcesIBL* resourcesIBL_;
	ResourcesShadow* resourcesShadow_;
	ResourcesGBuffer* resourcesGBuffer_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;
	std::vector<VulkanBuffer> shadowMapConfigUBOBuffers_;
	VulkanDescriptorSetInfo descriptorSetInfo_;

	// Material pass
	MaterialType materialType_;
	VkDeviceSize materialOffset_;
	uint32_t materialDrawCount_;

	// Specialization constants
	uint32_t alphaDiscard_;
};

#endif