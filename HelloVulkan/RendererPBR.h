#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "MeshCreateInfo.h"
#include "Mesh.h"

class RendererPBR final : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		VulkanImage* depthImage,
		VulkanTexture* envMap,
		VulkanTexture* irradianceMap,
		const std::vector<MeshCreateInfo>& meshInfos);

	virtual ~RendererPBR();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

public:
	// TODO change this to private
	std::vector<Mesh> meshes_;

private:
	bool CreateDescriptorLayout(VulkanDevice& vkDev);
	bool CreateDescriptorSet(VulkanDevice& vkDev, Mesh& mesh);

	// Textures
	void LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap);

	void LoadMesh(VulkanDevice& vkDev, const MeshCreateInfo& info);

private:
	VulkanTexture* envMap_;
	VulkanTexture* irradianceMap_;
	VulkanTexture brdfLUT_;
};

#endif
