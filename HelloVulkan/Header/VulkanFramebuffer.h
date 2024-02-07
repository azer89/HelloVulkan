#ifndef VULKAN_FRAMEBUFFER
#define VULKAN_FRAMEBUFFER

#include "VulkanDevice.h"
#include "VulkanImage.h"

/*
Containing a single or multiple framebuffers.
*/
class VulkanFramebuffer
{
public:
	VulkanFramebuffer():
		device_(VK_NULL_HANDLE),
		framebufferCount_(0),
		framebufferInfo_({}),
		attachmentImages_({}),
		framebuffers_({}),
		offscreen_(false)
	{
	}

	void Create(VulkanDevice& vkDev,
		VkRenderPass renderPass,
		const std::vector<VulkanImage*>& attachmentImage,
		bool offscreen);

	void Destroy();

	VkFramebuffer GetFramebuffer() const;

	VkFramebuffer GetFramebuffer(size_t index) const;

	void Recreate(VulkanDevice& vkDev);

private:
	std::vector<VkFramebuffer> framebuffers_;
	std::vector<VulkanImage*> attachmentImages_;
	VkFramebufferCreateInfo framebufferInfo_; // Caching creation info
	VkDevice device_;
	uint32_t framebufferCount_;
	bool offscreen_;
};

#endif