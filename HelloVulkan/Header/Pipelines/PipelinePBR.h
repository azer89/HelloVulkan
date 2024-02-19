#ifndef PIPELINE_PBR
#define PIPELINE_PBR

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "IBLResources.h"
#include "Model.h"
#include "Light.h"

#include <vector>

/*
Render meshes using PBR materials, naive forward renderer
*/
class PipelinePBR final : public PipelineBase
{
public:
	PipelinePBR(VulkanContext& ctx,
		std::vector<Model*> models,
		Lights* lights,
		//VulkanImage* specularMap,
		//VulkanImage* diffuseMap,
		//VulkanImage* brdfLUT,
		IBLResources* iblResources,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBR();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstantPBR& pbrPC) { pc_ = pbrPC; };

private:
	void CreateDescriptor(VulkanContext& ctx);
	void CreateDescriptorSet(VulkanContext& ctx, Model* parentModel, Mesh* mesh, const size_t meshIndex);

	Lights* lights_;

	// Image-Based Lighting
	// TODO Organize these inside a struct
	//VulkanImage* specularCubemap_;
	//VulkanImage* diffuseCubemap_;
	//VulkanImage* brdfLUT_;
	IBLResources* iblResources_;

	PushConstantPBR pc_;

	std::vector<Model*> models_;
	std::vector<std::vector<VkDescriptorSet>> descriptorSets_;
};

#endif
