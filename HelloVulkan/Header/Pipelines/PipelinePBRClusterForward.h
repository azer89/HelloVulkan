#ifndef PIPELINE_PBR_CLUSTER_FORWARD
#define PIPELINE_PBR_CLUSTER_FORWARD

#include "PipelinePBR.h"
#include "ClusterForwardBuffers.h"

/*
Render meshes using PBR materials, clustered forward renderer
*/
class PipelinePBRClusterForward final : public PipelinePBR
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

	void SetClusterForwardUBO(VulkanDevice& vkDev, ClusterForwardUBO ubo)
	{
		size_t currentImage = vkDev.GetCurrentSwapchainImageIndex();
		cfUBOBuffers_[currentImage].UploadBufferData(vkDev, 0, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	ClusterForwardBuffers* cfBuffers_;
	std::vector<VulkanBuffer> cfUBOBuffers_;

protected:
	void InitExtraResources(VulkanDevice& vkDev) override;
	void CreatePBRPipeline(VulkanDevice& vkDev) override;
	void CreateDescriptor(VulkanDevice& vkDev) override;
	void CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh) override;
};

#endif