#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "MeshCreateInfo.h"
#include "Mesh.h"

class RendererPBR : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		VulkanImage* depthImage,
		VulkanTexture* cubemapTexture,
		const std::vector<MeshCreateInfo>& meshInfos,
		const char* texIrrMapFile);

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
	VulkanTexture* cubemapTexture_;
	VulkanTexture envMapIrradiance_;
	VulkanTexture brdfLUT_;
	
};

#endif
