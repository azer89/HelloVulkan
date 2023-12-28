#ifndef RENDERER_CUBE
#define RENDERER_CUBE

#include "RendererBase.h"
#include "VulkanImage.h"

#include "glm/glm.hpp"

class RendererSkybox final : public RendererBase
{
public:
	RendererSkybox(VulkanDevice& vkDev, 
		VulkanImage* envMap,
		VulkanImage* depthImage);
	virtual ~RendererSkybox();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanImage* envMap_;

	std::vector<VkDescriptorSet> descriptorSets_;

	bool CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);
};

#endif
