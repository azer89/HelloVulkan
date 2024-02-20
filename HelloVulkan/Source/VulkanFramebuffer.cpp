#include "VulkanFramebuffer.h"
#include "VulkanUtility.h"

void VulkanFramebuffer::CreateResizeable(VulkanContext& ctx,
	VkRenderPass renderPass,
	const std::vector<VulkanImage*>& attachmentImages,
	bool offscreen)
{
	resizeable_ = true;
	offscreen_ = offscreen;
	device_ = ctx.GetDevice();
	attachmentImages_ = attachmentImages;
	framebufferCount_ = offscreen_ ? 1u : static_cast<uint32_t>(ctx.GetSwapchainImageCount());
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

	// Create framebuffer in this function
	Recreate(ctx);
}

void VulkanFramebuffer::CreateUnresizeable(
	VulkanContext& ctx,
	VkRenderPass renderPass,
	const std::vector<VkImageView>& attachments,
	uint32_t width,
	uint32_t height)
{
	resizeable_ = false;
	device_ = ctx.GetDevice();

	framebufferInfo_ =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.renderPass = renderPass,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.width = width,
		.height = height,
		.layers = 1u
	};
	framebuffers_.resize(1);
	VK_CHECK(vkCreateFramebuffer(ctx.GetDevice(), &framebufferInfo_, nullptr, &framebuffers_[0]));
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

void VulkanFramebuffer::Recreate(VulkanContext& ctx)
{
	if (!resizeable_)
	{
		std::cerr << "Cannot resize framebuffer\n";
	}

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

	framebufferInfo_.width = ctx.GetFrameBufferWidth();
	framebufferInfo_.height = ctx.GetFrameBufferHeight();

	for (size_t i = 0; i < framebufferCount_; ++i)
	{
		if (swapchainImageCount > 0)
		{
			attachments[0] = ctx.GetSwapchainImageView(i);
		}

		framebufferInfo_.attachmentCount = static_cast<uint32_t>(attachmentLength);
		framebufferInfo_.pAttachments = attachments.data();
		VK_CHECK(vkCreateFramebuffer(device_, &framebufferInfo_, nullptr, &framebuffers_[i]));
	}
}