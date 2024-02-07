#include "VulkanFramebuffer.h"
#include "VulkanUtility.h"

void VulkanFramebuffer::Create(VulkanDevice& vkDev,
	VkRenderPass renderPass,
	const std::vector<VulkanImage*>& attachmentImages,
	bool offscreen)
{
	offscreen_ = offscreen;
	device_ = vkDev.GetDevice();
	attachmentImages_ = attachmentImages;
	framebufferCount_ = offscreen_ ? 1u : static_cast<uint32_t>(vkDev.GetSwapchainImageCount());
	framebuffers_.resize(framebufferCount_);

	if (offscreen_ && attachmentImages_.empty())
	{
		std::cerr << "Need at least one image attachment to create a framebuffer\n";
	}

	// Create info
	framebufferInfo_ = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass,
		// Need to set these four in Recreate()
		/*.attachmentCount =,
		.pAttachments =,
		.width = w,
		.height = h,*/
		.layers = 1
	};

	Recreate(vkDev);
}

void VulkanFramebuffer::Destroy()
{
	for (VkFramebuffer& f : framebuffers_)
	{
		vkDestroyFramebuffer(device_, f, nullptr);
	}
	// NOTE Don't clear because we may recreate
	//framebuffers_.clear();
}

VkFramebuffer VulkanFramebuffer::GetFramebuffer() const
{
	return GetFramebuffer(0);
}

VkFramebuffer VulkanFramebuffer::GetFramebuffer(size_t index) const
{
	if (index < framebuffers_.size())
	{
		return framebuffers_[index];
	}
	return VK_NULL_HANDLE;
}

void VulkanFramebuffer::Recreate(VulkanDevice& vkDev)
{
	const size_t swapchainImageCount = offscreen_ ? 0 : 1;
	const size_t attachmentLength = attachmentImages_.size() + swapchainImageCount;

	if (attachmentLength <= 0)
	{
		return;
	}

	std::vector<VkImageView> attachments(attachmentLength, VK_NULL_HANDLE);
	for (size_t i = 0; i < attachmentImages_.size(); ++i)
	{
		attachments[i + swapchainImageCount] = attachmentImages_[i]->imageView_;
	}

	framebufferInfo_.width = vkDev.GetFrameBufferWidth();
	framebufferInfo_.height = vkDev.GetFrameBufferHeight();

	for (size_t i = 0; i < framebufferCount_; ++i)
	{
		if (swapchainImageCount > 0)
		{
			attachments[0] = vkDev.GetSwapchainImageView(i);
		}

		framebufferInfo_.attachmentCount = static_cast<uint32_t>(attachmentLength);
		framebufferInfo_.pAttachments = attachments.data();
		VK_CHECK(vkCreateFramebuffer(device_, &framebufferInfo_, nullptr, &framebuffers_[i]));
	}
}