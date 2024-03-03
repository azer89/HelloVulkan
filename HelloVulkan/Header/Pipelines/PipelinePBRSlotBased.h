#ifndef PIPELINE_PBR_SLOT_BASED
#define PIPELINE_PBR_SLOT_BASED

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "Model.h"
#include "ResourcesShared.h"
#include "ResourcesLight.h"
#include "ResourcesIBL.h"

#include <vector>

/*
Render meshes using PBR materials, naive forward renderer
*/
class PipelinePBRSlotBased final : public PipelineBase
{
public:
	PipelinePBRSlotBased(VulkanContext& ctx,
		const std::vector<Model*>& models,
		ResourcesLight* resLight,
		ResourcesIBL* iblResources,
		ResourcesShared* resShared,
		uint8_t renderBit = 0u);
	 ~PipelinePBRSlotBased();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };

private:
	void CreateDescriptor(VulkanContext& ctx);

	PushConstPBR pc_;
	ResourcesLight* resLight_;
	ResourcesIBL* iblResources_;
	std::vector<Model*> models_;
	std::vector<std::vector<VkDescriptorSet>> descriptorSets_;
};

#endif
