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

public:
	// TODO change this to private
	Mesh mesh_;

private:
	bool CreateDescriptorLayout(VulkanDevice& vkDev);
	bool CreateDescriptorSet(VulkanDevice& vkDev, Mesh& mesh);

	// Textures
	void LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap);

private:
	VulkanTexture envMap_;
	VulkanTexture envMapIrradiance_;
	VulkanTexture brdfLUT_;
};

#endif
