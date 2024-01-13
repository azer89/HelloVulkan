#include "RendererImGui.h"

RendererImGui::RendererImGui(VulkanDevice& vkDev) :
	RendererBase(vkDev, false) // Onscreen
{

}

RendererImGui::~RendererImGui()
{

}

void RendererImGui::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{

}