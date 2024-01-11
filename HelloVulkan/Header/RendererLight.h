#ifndef RENDERER_LIGHT
#define RENDERER_LIGHT

#include "VulkanDevice.h"
#include "RendererBase.h"
#include "Light.h"

class RendererLight final : public RendererBase
{
public:
	RendererLight(
		VulkanDevice& vkDev,
		std::vector<Light>& lights
	);
	~RendererLight();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	std::vector<Light> lights_;
};

#endif