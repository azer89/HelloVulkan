#include "RendererClear.h"
#include "VulkanUtility.h"

// Clear color and depth
RendererClear::RendererClear(VulkanDevice& vkDev, VulkanImage* depthImage) :
	RendererBase(vkDev, depthImage)
{
	/*renderPass_.CreateOnScreenRenderPass(
		vkDev,
		// Clear color and depth
		RenderPassBit::OnScreenColorClear | RenderPassBit::OnScreenDepthClear);
	*/
	renderPass_.CreateOnScreenClearColorRenderPass(vkDev);
	//clearDepthRenderPass_.CreateOnScreenClearDepthRenderPass(vkDev);

	/*CreateOnScreenFramebuffers(
		vkDev, 
		renderPass_, 
		depthImage_->imageView_);*/
	CreateOnScreenFramebuffers(
		vkDev,
		renderPass_);
}

RendererClear::~RendererClear()
{
	//clearColorRenderPass_.Destroy(device_);
	//clearDepthRenderPass_.Destroy(device_);
}

void RendererClear::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(commandBuffer, swapchainFramebuffers_[swapchainImageIndex]);
	vkCmdEndRenderPass(commandBuffer);
}