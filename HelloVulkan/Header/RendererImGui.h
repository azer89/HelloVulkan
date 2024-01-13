#ifndef RENDERER_IMGUI
#define RENDERER_IMGUI

#include "RendererBase.h"

class RendererImGui final : public RendererBase
{
	RendererImGui(VulkanDevice& vkDev);
	~RendererImGui();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif