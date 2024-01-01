#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanImage.h"
#include "Model.h"

class RendererPBR final : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		std::vector<Model*> models,
		VulkanImage* depthImage,
		VulkanImage* envMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		VulkanImage* offscreenColorImage = nullptr,
		uint8_t renderBit = 0u);
	 ~RendererPBR();

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh);

private:
	VulkanImage* envMap_;
	VulkanImage* diffuseMap_;
	VulkanImage* brdfLUT_;

	//VulkanImage* offscreenColorImage_;
	//VkFramebuffer offscreenFramebuffer_;
};

#endif
