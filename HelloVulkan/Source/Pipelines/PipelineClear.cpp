#include "PipelineClear.h"
#include "VulkanUtility.h"

PipelineClear::PipelineClear(VulkanDevice& vkDev) :
	PipelineBase(
		vkDev,
		{
		.flags_ = PipelineFlags::GraphicsOnScreen
		})
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev, RenderPassBit::ColorClear);
	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, IsOffscreen());
}

PipelineClear::~PipelineClear()
{
}

void PipelineClear::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	vkCmdEndRenderPass(commandBuffer);
}