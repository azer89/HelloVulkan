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
class PipelinePBR : public PipelineBase
{
public:
	PipelinePBR(VulkanDevice& vkDev,
		std::vector<Model*> models,
		Lights* lights,
		VulkanImage* specularMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~PipelinePBR();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

	void SetLightIntensity(float intensity) { pc_.lightIntensity = intensity; }
	void SetBaseReflectivity(float baseReflectivity) { pc_.baseReflectivity = baseReflectivity; }
	void SetMaxReflectionLod(float maxLod) { pc_.maxReflectionLod = maxLod; }
	void SetLightFalloff(float lightFalloff) { pc_.lightFalloff = lightFalloff; }
	void SetAlbedoMultipler(float albedoMultipler) { pc_.albedoMultipler = albedoMultipler; }

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	void CreateDescriptor(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh);

	Lights* lights_;

	// Image-Based Lighting
	// TODO Organize these inside a struct
	VulkanImage* specularCubemap_;
	VulkanImage* diffuseCubemap_;
	VulkanImage* brdfLUT_;

	PushConstantPBR pc_;
};

#endif
