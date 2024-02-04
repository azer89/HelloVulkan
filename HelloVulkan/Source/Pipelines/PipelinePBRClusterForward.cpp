#include "PipelinePBRClusterForward.h"

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 1;
constexpr uint32_t PBR_MESH_TEXTURE_COUNT = 6;
constexpr uint32_t PBR_ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PipelinePBRClusterForward::PipelinePBRClusterForward(
	VulkanDevice& vkDev,
	std::vector<Model*> models,
	Lights* lights,
	VulkanImage* specularMap,
	VulkanImage* diffuseMap,
	VulkanImage* brdfLUT,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit = 0u) :
	PipelinePBR(
		vkDev,
		models,
		lights,
		specularMap,
		diffuseMap,
		brdfLUT,
		depthImage,
		offscreenColorImage,
		renderBit
	)
{
}

void PipelinePBRClusterForward::CreatePBRPipeline(VulkanDevice& vkDev)
{
}

void PipelinePBRClusterForward::CreateDescriptor(VulkanDevice& vkDev)
{
}

void PipelinePBRClusterForward::CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh)
{
}