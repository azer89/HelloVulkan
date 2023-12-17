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
		uint32_t uniformBufferSize,
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

	//void UpdateUniformBuffer(VulkanDevice& vkDev, uint32_t currentImage, const void* data, const size_t dataSize);

private:
	bool CreateDescriptorSet(VulkanDevice& vkDev, uint32_t uniformDataSize);

	// Textures
	void LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap);

private:
	Mesh mesh_;

	VulkanTexture envMap_;
	VulkanTexture envMapIrradiance_;
	VulkanTexture brdfLUT_;
};

#endif
