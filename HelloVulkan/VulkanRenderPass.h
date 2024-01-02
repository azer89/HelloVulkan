#ifndef VULKAN_RENDER_PASS
#define VULKAN_RENDER_PASS

#include "VulkanDevice.h"

enum RenderPassBit : uint8_t
{
	// Clear color attachment
	OffScreenColorClear = 0x01,

	// Clear depth attachment
	OffScreenDepthClear = 0x02,

	// Transition color attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	// for the next onscreen render pass
	OffScreenColorShaderReadOnly = 0x04,

	// Clear swapchain color attachment
	OnScreenColorClear = 0x08,

	// Clear depth attachment
	OnScreenDepthClear = 0x10,

	// Present swapchain color attachment as VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	OnScreenColorPresent = 0x20
};

class VulkanRenderPass
{
public:
	VulkanRenderPass() = default;
	~VulkanRenderPass() = default;

	void CreateOnScreenRenderPass(
		VulkanDevice& device, 
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOnScreenColorOnlyRenderPass(
		VulkanDevice& device, 
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOffScreenRenderPass(
		VulkanDevice& device,
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOffScreenCubemapRenderPass(
		VulkanDevice& device, 
		VkFormat cubeFormat,
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	// There's no EndRenderPass() function because you can just call vkCmdEndRenderPass(commandBuffer);
	void BeginRenderPass(
		VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer);

	void BeginCubemapRenderPass(
		VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer,
		uint32_t cubeSideLength);

	void Destroy(VkDevice vkDev);

	VkRenderPass GetHandle() { return handle_;}

private:
	void CreateBeginInfo(VulkanDevice& device);

private:
	VkRenderPass handle_;
	uint8_t renderPassBit_;

	// Cache for starting the render pass
	std::vector<VkClearValue> clearValues_;
	VkRenderPassBeginInfo beginInfo_;
};

#endif
