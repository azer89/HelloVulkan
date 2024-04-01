#ifndef PIPELINE_PBR_SLOT_BASED
#define PIPELINE_PBR_SLOT_BASED

#include "PipelineBase.h"
#include "ResourcesShared.h"
#include "ResourcesLight.h"
#include "ResourcesIBL.h"
#include "Model.h"
#include "PushConstants.h"

#include <vector>

/*
Render meshes using PBR materials, naive forward renderer
*/
class PipelinePBRSlotBased final : public PipelineBase
{
public:
	PipelinePBRSlotBased(VulkanContext& ctx,
		const std::vector<Model*>& models,
		ResourcesLight* resourcesLight,
		ResourcesIBL* resourcesIBL,
		ResourcesShared* resourcesShared,
		uint8_t renderBit = 0u);
	 ~PipelinePBRSlotBased();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };

	void UpdateFromIUData(VulkanContext& ctx, UIData& uiData) override
	{
		pc_ = uiData.pbrPC_;
	}

private:
	void CreateDescriptor(VulkanContext& ctx);

	PushConstPBR pc_;
	ResourcesLight* resourcesLight_;
	ResourcesIBL* resourcesIBL_;
	std::vector<Model*> models_;
	std::vector<std::vector<VkDescriptorSet>> descriptorSets_;
};

#endif
