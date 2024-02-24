#ifndef PIPELINE_PBR_BINDLESS
#define PIPELINE_PBR_BINDLESS

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "IBLResources.h"
#include "Scene.h"
#include "Light.h"

#include <vector>

/*struct PerModelBindlessResource
{
	VulkanDescriptor descriptor_;
	std::vector<VkDescriptorSet> descriptorSets_;
	std::vector<VulkanBuffer> indirectBuffers_;

	void Destroy()
	{
		descriptor_.Destroy();
		for (VulkanBuffer& buffer : indirectBuffers_)
		{
			buffer.Destroy();
		}
	}
};*/

/*
Render meshes using PBR materials, naive forward renderer, bindless
*/
class PipelinePBRBindless final : public PipelineBase
{
public:
	PipelinePBRBindless(VulkanContext& ctx,
		//std::vector<Model*> models,
		Scene* scene,
		Lights* lights,
		IBLResources* iblResources,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBRBindless();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstantPBR& pbrPC) { pc_ = pbrPC; };

private:
	void CreateIndirectBuffers(VulkanContext& ctx);

	void CreateDescriptor(VulkanContext& ctx);
	//void CreateDescriptorSet(VulkanContext& ctx, Model* model, size_t modelIndex);

	PushConstantPBR pc_;
	Lights* lights_;
	IBLResources* iblResources_;
	//std::vector<Model*> models_;
	Scene* scene_;
	VulkanBuffer indirectBuffer_;
	//std::vector<PerModelBindlessResource> modelResources_;
};

#endif
