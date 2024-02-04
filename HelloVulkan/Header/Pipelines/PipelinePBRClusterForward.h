#ifndef PIPELINE_PBR_CLUSTER_FORWARD
#define PIPELINE_PBR_CLUSTER_FORWARD

#include "PipelinePBR.h"
#include "ClusterForwardBuffers.h"

/*
Render meshes using PBR materials, clustered forward renderer
*/
class PipelinePBRClusterForward final : public PipelineBase
{
public:
	PipelinePBRClusterForward(VulkanDevice& vkDev,
		std::vector<Model*> models,
		Lights* lights,
		ClusterForwardBuffers* cfBuffers,
		VulkanImage* specularMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);
	~PipelinePBRClusterForward();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

	void SetLightIntensity(float intensity) { pc_.lightIntensity = intensity; }
	void SetBaseReflectivity(float baseReflectivity) { pc_.baseReflectivity = baseReflectivity; }
	void SetMaxReflectionLod(float maxLod) { pc_.maxReflectionLod = maxLod; }
	void SetLightFalloff(float lightFalloff) { pc_.lightFalloff = lightFalloff; }
	void SetAlbedoMultipler(float albedoMultipler) { pc_.albedoMultipler = albedoMultipler; }

	void SetClusterForwardUBO(VulkanDevice& vkDev, ClusterForwardUBO ubo)
	{
		size_t currentImage = vkDev.GetCurrentSwapchainImageIndex();
		cfUBOBuffers_[currentImage].UploadBufferData(vkDev, 0, &ubo, sizeof(ClusterForwardUBO));
	}

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	ClusterForwardBuffers* cfBuffers_;
	std::vector<VulkanBuffer> cfUBOBuffers_;

	Lights* lights_;

	// Image-Based Lighting
	// TODO Organize these inside a struct
	VulkanImage* specularCubemap_;
	VulkanImage* diffuseCubemap_;
	VulkanImage* brdfLUT_;

	PushConstantPBR pc_;

private:
	void CreateDescriptor(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh);
};

#endif