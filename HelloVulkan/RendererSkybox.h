#ifndef RENDERER_CUBE
#define RENDERER_CUBE

#include "RendererBase.h"
#include "VulkanTexture.h"

#include "glm/glm.hpp"

class RendererSkybox final : public RendererBase
{
public:
	RendererSkybox(VulkanDevice& vkDev, 
		VulkanTexture* envMap, 
		VulkanImage* depthImage);
	virtual ~RendererSkybox();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanTexture* envMap_;

	std::vector<VkDescriptorSet> descriptorSets_;

	bool CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);
};

#endif
