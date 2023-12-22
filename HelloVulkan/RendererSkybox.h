#ifndef RENDERER_CUBE
#define RENDERER_CUBE

#include "RendererBase.h"
#include "VulkanTexture.h"

#include "glm/glm.hpp"

class RendererSkybox final : public RendererBase
{
public:
	RendererSkybox(VulkanDevice& vkDev, 
		VulkanTexture* skyboxTexture, 
		VulkanImage* depthImage);
	virtual ~RendererSkybox();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanTexture* skyboxTexture_;

	std::vector<VkDescriptorSet> descriptorSets_;

	bool CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);
};

#endif
