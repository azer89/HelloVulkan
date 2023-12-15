#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"

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

	void UpdateUniformBuffer(VulkanDevice& vkDev, uint32_t currentImage, const void* data, const size_t dataSize);

private:
	bool CreateDescriptorSet(VulkanDevice& vkDev, uint32_t uniformDataSize);

	// ASSIMP
	bool CreateVertexBuffer(
		VulkanDevice& vkDev, 
		const char* filename, 
		VulkanBuffer* storageBuffer, 
		size_t* vertexBufferSize, 
		size_t* indexBufferSize);

	// Textures
	void LoadTexture(VulkanDevice& vkDev, const char* fileName, VulkanTexture& texture);
	void LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap);

private:
	size_t vertexBufferSize_;
	size_t indexBufferSize_;

	VulkanBuffer storageBuffer_;

	VulkanTexture texAO_;
	VulkanTexture texEmissive_;
	VulkanTexture texAlbedo_;
	VulkanTexture texMeR_;
	VulkanTexture texNormal_;

	VulkanTexture envMapIrradiance_;
	VulkanTexture envMap_;

	VulkanTexture brdfLUT_;
};

#endif
