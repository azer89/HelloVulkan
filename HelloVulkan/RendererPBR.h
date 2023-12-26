#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "Model.h"

class RendererPBR final : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		VulkanImage* depthImage,
		VulkanTexture* envMap,
		VulkanTexture* diffuseMap,
		VulkanTexture* brdfLUT,
		std::vector<Model*> models);

	virtual ~RendererPBR();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	bool CreateDescriptorLayout(VulkanDevice& vkDev);
	bool CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh);

private:
	VulkanTexture* envMap_;
	VulkanTexture* diffuseMap_;
	VulkanTexture* brdfLUT_;
};

#endif
