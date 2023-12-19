#ifndef RENDERER_CUBE
#define RENDERER_CUBE

#include "RendererBase.h"
#include "VulkanTexture.h"

#include "glm/glm.hpp"

class RendererSkybox : public RendererBase
{
public:
	RendererSkybox(VulkanDevice& vkDev, VulkanImage inDepthTexture, const char* textureFile);
	virtual ~RendererSkybox();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanTexture texture_;

	std::vector<VkDescriptorSet> descriptorSets_;

	bool CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);
};

#endif
