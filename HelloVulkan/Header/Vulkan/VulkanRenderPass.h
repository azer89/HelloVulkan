#ifndef VULKAN_RENDER_PASS
#define VULKAN_RENDER_PASS

#include "VulkanContext.h"

enum RenderPassBit : uint8_t
{
	// Clear color attachment
	ColorClear = 0x01,

	// Clear depth attachment
	DepthClear = 0x02,

	// Transition color attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	ColorShaderReadOnly = 0x04,

	// Transition color attachment to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	ColorPresent = 0x08,

	// Transition depth attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	DepthShaderReadOnly = 0x10,
};

class VulkanRenderPass
{
public:
	VulkanRenderPass() = default;
	~VulkanRenderPass() = default;

	// TODO Remove "RenderPass" wording at the end of the functions  

	void CreateOnScreenRenderPass(
		VulkanContext& ctx, 
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateDepthOnlyRenderPass(
		VulkanContext& ctx,
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateResolveMSRenderPass(
		VulkanContext& ctx,
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOnScreenColorOnlyRenderPass(
		VulkanContext& ctx, 
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOffScreenRenderPass(
		VulkanContext& ctx,
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOffScreenCubemapRenderPass(
		VulkanContext& ctx, 
		VkFormat cubeFormat,
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	// Multi Render Target for G Buffer
	void CreateOffScreenGBuffer(
		VulkanContext& ctx,
		const std::vector<VkFormat>& formats,
		uint8_t renderPassBit = 0u,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	// There's no EndRenderPass() function because you can just call vkCmdEndRenderPass(commandBuffer);
	void BeginRenderPass(
		VulkanContext& ctx,
		VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer);

	void BeginRenderPass(
		VulkanContext& ctx,
		VkCommandBuffer commandBuffer,
		VkFramebuffer framebuffer,
		uint32_t width,
		uint32_t height);

	void BeginCubemapRenderPass(
		VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer,
		uint32_t cubeSideLength);

	void Destroy();

	VkRenderPass GetHandle() const { return handle_;}

private:
	void CreateBeginInfo();

private:
	VkDevice device_ = nullptr;
	VkRenderPass handle_ = nullptr;
	uint8_t renderPassBit_;

	// TODO Refactor this
	uint32_t colorAttachmentCount_ = 1;

	// Cache for starting the render pass
	std::vector<VkClearValue> clearValues_;
	VkRenderPassBeginInfo beginInfo_;
};

#endif