#ifndef PIPELINE_PBR_CLUSTER_FORWARD
#define PIPELINE_PBR_CLUSTER_FORWARD

#include "PipelinePBR.h"

/*
Render meshes using PBR materials, clustered forward renderer
*/
class PipelinePBRClusterForward final : public PipelinePBR
{
public:
	PipelinePBRClusterForward(VulkanDevice& vkDev,
		std::vector<Model*> models,
		Lights* lights,
		VulkanImage* specularMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u);

private:
	void CreatePBRPipeline(VulkanDevice& vkDev) override;
	void CreateDescriptor(VulkanDevice& vkDev) override;
	void CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh) override;
};

#endif