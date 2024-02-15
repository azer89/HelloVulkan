#include "PipelineClear.h"
#include "VulkanUtility.h"

PipelineClear::PipelineClear(VulkanContext& vkDev) :
	PipelineBase(
		vkDev,
		{
			.type_ = PipelineType::GraphicsOnScreen
		})
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev, RenderPassBit::ColorClear);
	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, IsOffscreen());
}

PipelineClear::~PipelineClear()
{
}

void PipelineClear::FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer)
{
	uint32_t swapchainImageIndex = vkDev.GetCurrentSwapchainImageIndex();
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	vkCmdEndRenderPass(commandBuffer);
}