#ifndef RENDERER_EQUIRECT_2_CUBEMAP
#define RENDERER_EQUIRECT_2_CUBEMAP

#include "RendererBase.h"
#include "VulkanTexture.h"

#include "glm/glm.hpp"

class RendererEquirect2Cubemap : public RendererBase
{
public:
	RendererEquirect2Cubemap(VulkanDevice& vkDev);
	~RendererEquirect2Cubemap();

private:
	std::vector<VkDescriptorSet> descriptorSets_;

	VulkanTexture cubemapTexture_;

private:
	void InitializeCubemapTexture(VulkanDevice& vkDev);
	void CreateRenderPass(VulkanDevice& vkDev);
};

#endif
