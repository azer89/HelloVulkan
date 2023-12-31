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
		VulkanImage* specularMap,
		VulkanImage* diffuseMap,
		VulkanImage* brdfLUT,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage = nullptr,
		uint8_t renderBit = 0u);
	 ~RendererPBR();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	void OnWindowResized(VulkanDevice& vkDev) override;

public:
	// TODO change this to private
	std::vector<Model*> models_;

private:
	void CreateDescriptorLayout(VulkanDevice& vkDev);
	void CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh);

private:
	VulkanImage* specularMap_;
	VulkanImage* diffuseMap_;
	VulkanImage* brdfLUT_;
};

#endif
