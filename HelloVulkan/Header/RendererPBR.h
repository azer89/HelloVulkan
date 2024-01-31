#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanImage.h"
#include "Model.h"
#include "Light.h"
#include "ClusterForwardBuffers.h"

struct PushConstantPBR
{
	float lightIntensity = 1.f;
	float baseReflectivity = 0.04f;
	float maxReflectionLod = 4.f;
	float attenuationF = 1.f;
	float ambientStrength = 1.0f;
};

class RendererPBR final : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		std::vector<Model*> models,
		Lights* lights,
		ClusterForwardBuffers* cfBuffers,
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
	void SetAtenuationF(float f) { pc_.attenuationF = f; }
	void SetAmbientStrength(float f) { pc_.ambientStrength = f; }

	void SeClusterForwardUBO(const VulkanDevice& vkDev, uint32_t imageIndex, ClusterForwardUBO ubo)
	{
		UpdateUniformBuffer(vkDev.GetDevice(), clusterForwardUBOs_[imageIndex], &ubo, sizeof(ClusterForwardUBO));
	}

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh);

	Lights* lights_;
	ClusterForwardBuffers* cfBuffers_;

	// Image-Based Lighting
	VulkanImage* specularCubemap_;
	VulkanImage* diffuseCubemap_;
	VulkanImage* brdfLUT_;

	PushConstantPBR pc_;

	std::vector<VulkanBuffer> clusterForwardUBOs_;
};

#endif
