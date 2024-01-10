#include "VulkanFramebuffer.h"
#include "VulkanUtility.h"

void VulkanFramebuffer::Create(VulkanDevice& vkDev,
	VkRenderPass renderPass,
	const std::vector<VulkanImage*> swapchainImages,
	const std::vector<VulkanImage*> images)
{
	if (swapchainImages.size() == 0 && images.size() == 0)
	{
		std::cerr << "No framebuffer attachment\n";
	}

	device_ = vkDev.GetDevice();
	swapchainImages_ = swapchainImages;
	images_ = images;
	framebufferCount_ = swapchainImages.size() > 0 ? swapchainImages.size() : 1;
	framebuffers_.resize(framebufferCount_);

	uint32_t w = vkDev.GetFrameBufferWidth();
	uint32_t h = vkDev.GetFrameBufferHeight();

	// Create info
	framebufferInfo_ = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass,
		// Need to set these four in Recreate()
		//.attachmentCount =,
		//.pAttachments =,
		//.width = w,
		//.height = h,
		.layers = 1
	};

	Recreate(w, h);
}

void VulkanFramebuffer::Destroy()
{
	for (VkFramebuffer& f : framebuffers_)
	{
		vkDestroyFramebuffer(device_, f, nullptr);
	}
	framebuffers_.clear();
}

VkFramebuffer VulkanFramebuffer::GetFramebuffer() const
{
	return GetFramebuffer(0);
}

VkFramebuffer VulkanFramebuffer::GetFramebuffer(size_t index) const
{
	if (index >= 0 && index < framebuffers_.size())
	{
		return framebuffers_[index];
	}
	return VK_NULL_HANDLE;
}

void VulkanFramebuffer::Recreate(uint32_t width, uint32_t height)
{
	size_t swapchainImageCount = swapchainImages_.size() > 0 ? 1 : 0;
	size_t attachmentLength = images_.size() + swapchainImageCount;
	std::vector<VkImageView> attachments(attachmentLength, VK_NULL_HANDLE);
	for (size_t i = 0; i < images_.size(); ++i)
	{
		attachments[i + swapchainImageCount] = images_[i]->imageView_;
	}

	framebufferInfo_.width = width;
	framebufferInfo_.height = height;

	for (size_t i = 0; i < framebufferCount_; ++i)
	{
		if (swapchainImageCount > 0)
		{
			attachments[0] = swapchainImages_[i]->imageView_;
		}
		framebufferInfo_.attachmentCount = static_cast<uint32_t>(attachmentLength);
		framebufferInfo_.pAttachments = attachments.data();
		VK_CHECK(vkCreateFramebuffer(device_, &framebufferInfo_, nullptr, &framebuffers_[i]));
	}
}