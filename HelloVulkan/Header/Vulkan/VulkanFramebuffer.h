#ifndef VULKAN_FRAMEBUFFER
#define VULKAN_FRAMEBUFFER

#include "VulkanContext.h"
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

	void Destroy();

	// Can be recreated/resized
	void CreateResizeable(VulkanContext& ctx,
		VkRenderPass renderPass,
		const std::vector<VulkanImage*>& attachmentImage,
		bool offscreen);

	// Cannot be recreated/resized
	void CreateUnresizeable(
		VulkanContext& ctx,
		VkRenderPass renderPass,
		const std::vector<VkImageView>& attachments,
		uint32_t width,
		uint32_t height);

	[[nodiscard]] VkFramebuffer GetFramebuffer() const;
	[[nodiscard]] VkFramebuffer GetFramebuffer(size_t index) const;
	void Recreate(VulkanContext& ctx);

private:
	std::vector<VkFramebuffer> framebuffers_ = {};
	std::vector<VulkanImage*> attachmentImages_ = {};
	VkFramebufferCreateInfo framebufferInfo_ = {}; // Caching creation info
	VkDevice device_ = nullptr;
	uint32_t framebufferCount_ = 0;
	bool offscreen_ = false;
	bool resizeable_ = false;
};

#endif