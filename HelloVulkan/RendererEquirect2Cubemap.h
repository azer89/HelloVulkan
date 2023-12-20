#ifndef RENDERER_EQUIRECT_2_CUBEMAP
#define RENDERER_EQUIRECT_2_CUBEMAP

#include "RendererBase.h"
#include "VulkanTexture.h"

#include "glm/glm.hpp"

#include <string>

class RendererEquirect2Cubemap : public RendererBase
{
public:
	RendererEquirect2Cubemap(VulkanDevice& vkDev, const std::string& hdrFile);
	~RendererEquirect2Cubemap();

private:
	std::vector<VkDescriptorSet> descriptorSets_;

	VulkanTexture cubemapTexture_;
	VulkanTexture hdrTexture_;

private:
	void InitializeCubemapTexture(VulkanDevice& vkDev);
	void InitializeHDRTexture(VulkanDevice& vkDev, const std::string& hdrFile);
	void CreateRenderPass(VulkanDevice& vkDev);
	bool CreateDescriptorLayout(VulkanDevice& vkDev);
	bool CreateDescriptorSet(VulkanDevice& vkDev);
};

#endif
