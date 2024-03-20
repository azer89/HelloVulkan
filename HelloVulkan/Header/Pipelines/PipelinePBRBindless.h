#ifndef PIPELINE_PBR_BINDLESS_TEXTURES
#define PIPELINE_PBR_BINDLESS_TEXTURES

#include "PipelineBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "ResourcesIBL.h"
#include "ResourcesShared.h"
#include "PushConstants.h"

#include <vector>

/*
Render a scene using PBR materials, naive forward renderer, and bindless textures
*/
class PipelinePBRBindless final : public PipelineBase
{
public:
	PipelinePBRBindless(VulkanContext& ctx,
		Scene* scene,
		ResourcesLight* resLight,
		ResourcesIBL* iblResources,
		ResourcesShared* resShared,
		uint8_t renderBit = 0u);
	 ~PipelinePBRBindless();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void SetPBRPushConstants(const PushConstPBR& pbrPC) { pc_ = pbrPC; };

private:
	void PrepareVIM(VulkanContext& ctx);
	void CreateDescriptor(VulkanContext& ctx);

	Scene* scene_;
	ResourcesLight* resLight_;
	ResourcesIBL* iblResources_;
	PushConstPBR pc_;
	VulkanBuffer vimBuffer_;
	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif
