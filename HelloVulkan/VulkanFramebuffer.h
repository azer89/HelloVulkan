#ifndef VULKAN_FRAMEBUFFER
#define VULKAN_FRAMEBUFFER

#include "VulkanDevice.h"
#include "VulkanImage.h"

class VulkanFramebuffer
{
public:
	VulkanFramebuffer():
		device_(VK_NULL_HANDLE),
		framebufferCount_(0),
		framebufferInfo_({}),
		swapchainImages_({}),
		images_({}),
		framebuffers_({})
	{
	}

	void Create(VulkanDevice& vkDev,
		VkRenderPass renderPass,
		const std::vector<VulkanImage*> swapchainImages,
		const std::vector<VulkanImage*> images);

	void Destroy();

	VkFramebuffer GetFramebuffer() const;

	VkFramebuffer GetFramebuffer(size_t index) const;

	void Recreate(uint32_t width, uint32_t height);

private:
	std::vector<VkFramebuffer> framebuffers_;
	std::vector<VulkanImage*> swapchainImages_;
	std::vector<VulkanImage*> images_;
	VkFramebufferCreateInfo framebufferInfo_; // Caching creation info
	VkDevice device_;
	uint32_t framebufferCount_;
};

#endif