#ifndef PIPELINE_PBR
#define PIPELINE_PBR

#include "PipelineBase.h"
#include "VulkanImage.h"
#include "PushConstants.h"
#include "Model.h"
#include "Light.h"

/*
Render meshes using PBR materials, naive forward renderer
*/
class PipelinePBR final : public PipelineBase
{
public:
	PipelinePBR(VulkanContext& vkDev,
		std::vector<Model*> models,
		Lights* lights,
		VulkanImage* specularMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBR();

	void FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PushConstantPBR& pbrPC) { pc_ = pbrPC; };

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	void CreateDescriptor(VulkanContext& vkDev);
	void CreateDescriptorSet(VulkanContext& vkDev, Model* parentModel, Mesh& mesh);

	Lights* lights_;

	// Image-Based Lighting
	// TODO Organize these inside a struct
	VulkanImage* specularCubemap_;
	VulkanImage* diffuseCubemap_;
	VulkanImage* brdfLUT_;

	PushConstantPBR pc_;
};

#endif
