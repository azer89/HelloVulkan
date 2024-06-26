#ifndef PIPELINE_PBR_BINDLESS
#define PIPELINE_PBR_BINDLESS

#include "PipelineBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "ResourcesIBL.h"
#include "ResourcesShared.h"
#include "PushConstants.h"

#include <vector>

/*
Render a scene using PBR materials, naive forward renderer, and bindless techniques
*/
class PipelinePBRBindless final : public PipelineBase
{
public:
	PipelinePBRBindless(VulkanContext& ctx,
		Scene* scene,
		ResourcesLight* resourcesLight,
		ResourcesIBL* resourcesIBL,
		ResourcesShared* resourcesShared,
		bool useSkinning,
		uint8_t renderBit = 0u);
	 ~PipelinePBRBindless();

	 void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	
	void UpdateFromUIData(VulkanContext& ctx, UIData& uiData) override
	{
		SetPBRPushConstants(uiData.pbrPC_);
	}

private:
	void CreateBDABuffer(VulkanContext& ctx);
	void CreateDescriptor(VulkanContext& ctx);

	bool useSkinning_;
	Scene* scene_;
	ResourcesLight* resourcesLight_;
	ResourcesIBL* resourcesIBL_;
	PushConstPBR pc_;
	VulkanBuffer bdaBuffer_;
	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif
