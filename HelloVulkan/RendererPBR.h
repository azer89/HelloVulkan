#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "Mesh.h"

class RendererPBR : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		const char* modelFile,
		const char* texAOFile,
		const char* texEmissiveFile,
		const char* texAlbedoFile,
		const char* texMeRFile,
		const char* texNormalFile,
		const char* texEnvMapFile,
		const char* texIrrMapFile,
		VulkanImage depthTexture);

	virtual ~RendererPBR();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

	void SetModelUBO(const VulkanDevice& vkDev, uint32_t imageIndex, ModelUBO ubo)
	{
		UpdateUniformBuffer(vkDev.GetDevice(), modelBuffers_[imageIndex], &ubo, sizeof(ModelUBO));
	}

private:
	bool CreateDescriptorSet(VulkanDevice& vkDev);

	// Textures
	void LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap);

private:
	Mesh mesh_;
	// ModelUBO
	// TODO MOve this to mesh
	std::vector<VulkanBuffer> modelBuffers_; 

	VulkanTexture envMap_;
	VulkanTexture envMapIrradiance_;
	VulkanTexture brdfLUT_;
};

#endif
