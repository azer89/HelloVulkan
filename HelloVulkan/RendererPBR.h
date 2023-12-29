#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanImage.h"
#include "Model.h"

class RendererPBR final : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		VulkanImage* depthImage,
		VulkanImage* multisampledImage,
		VulkanImage* envMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		std::vector<Model*> models);
	 ~RendererPBR();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	bool CreateDescriptorLayout(VulkanDevice& vkDev);
	bool CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh);

private:
	VulkanImage* envMap_;
	VulkanImage* diffuseMap_;
	VulkanImage* brdfLUT_;

	VulkanImage* multisampledImage_;
};

#endif
