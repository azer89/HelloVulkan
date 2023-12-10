#ifndef RENDERER_CUBE
#define RENDERER_CUBE

#include "RendererBase.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class RendererCube : RendererBase
{
public:
	RendererCube(VulkanDevice& vkDev, VulkanImage inDepthTexture, const char* textureFile);
	virtual ~RendererCube();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

	void UpdateUniformBuffer(VulkanDevice& vkDev, uint32_t currentImage, const glm::mat4& m);

private:
	VkSampler textureSampler;
	VulkanImage texture;

	bool CreateDescriptorSet(VulkanDevice& vkDev);

	

};

#endif
