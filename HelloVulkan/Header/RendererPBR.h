#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanImage.h"
#include "Model.h"
#include "Light.h"

struct PushConstantPBR
{
	float lightIntensity = 1.f;
	float baseReflectivity = 0.04f;
	float maxReflectionLod = 4.f;
};

class RendererPBR final : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		std::vector<Model*> models,
		Lights* lights,
		VulkanImage* specularMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	 ~RendererPBR();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	void SetLightIntensity(float intensity) { pc_.lightIntensity = intensity; }
	void SetBaseReflectivity(float baseReflectivity) { pc_.baseReflectivity = baseReflectivity; }
	void SetMaxReflectionLod(float maxLod) { pc_.maxReflectionLod = maxLod; }

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh);

	Lights* lights_;

	// Image-Based Lighting
	VulkanImage* specularCubemap_;
	VulkanImage* diffuseCubemap_;
	VulkanImage* brdfLUT_;

	PushConstantPBR pc_;
};

#endif
