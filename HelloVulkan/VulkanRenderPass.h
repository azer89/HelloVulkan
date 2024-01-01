#ifndef VULKAN_RENDER_PASS
#define VULKAN_RENDER_PASS

#include "VulkanDevice.h"

enum RenderPassBit : uint8_t
{
	// Clear color attachment
	OffScreenColorClear = 0x01,

	// Transition color attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	// for the next onscreen render pass
	OffScreenColorShaderReadOnly = 0x02,

	// Clear swapchain color attachment
	OnScreenColorClear = 0x04,

	// Clear depth attachment
	OnScreenDepthClear = 0x08,

	// Present swapchain color attachment as VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	OnScreenColorPresent = 0x10
};

class VulkanRenderPass
{
public:
	VulkanRenderPass();
	~VulkanRenderPass();

	void CreateOffScreenRenderPass(VulkanDevice& device, uint8_t renderPassBit = 0u);

	void CreateOnScreenRenderPass(VulkanDevice& device, uint8_t renderPassBit = 0u);

	void BeginRenderPass(VulkanDevice& device, VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);

	void Destroy(VkDevice vkDev);

	VkRenderPass GetHandle() { return handle_;}
	//VkRenderPassBeginInfo* BeginInfoPtr() { return &beginInfo_; }

private:
	VkRenderPassBeginInfo CreateBeginInfo(VulkanDevice& device);

private:
	VkRenderPass handle_;
	//VkRenderPassBeginInfo beginInfo_;
	uint8_t renderPassBit_;

	//bool clearColor_;
	//bool presentColor_;
	//bool clearDepth_;
	//bool colorShaderReadOnly_;
};

#endif
