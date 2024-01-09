#ifndef VULKAN_FRAMEBUFFER
#define VULKAN_FRAMEBUFFER

#include "VulkanDevice.h"
#include "VulkanImage.h"

class VulkanFramebuffer
{
public:
	VulkanFramebuffer():
		device_(VK_NULL_HANDLE),
		framebufferCount_(0)
	{
	}

	void Create(VulkanDevice& vkDev,
		const std::vector<VulkanImage*> images,
		uint32_t framebufferCount)
	{
		device_ = vkDev.GetDevice();
		images_ = images;
		framebufferCount_ = framebufferCount;
		framebuffers_.resize(framebufferCount_);
	}

	void Destroy()
	{
		for (VkFramebuffer& f : framebuffers_)
		{
			vkDestroyFramebuffer(device_, f, nullptr);
		}
		framebuffers_.clear();
	}

	VkFramebuffer GetFramebuffer() const
	{
		return GetFramebuffer(0);
	}

	VkFramebuffer GetFramebuffer(size_t index) const
	{
		if (index >= 0 && index < framebuffers_.size())
		{
			return framebuffers_[index];
		}

		return VK_NULL_HANDLE;
	}

	void Recreate(uint32_t width, uint32_t height)
	{
		for (size_t i = 0; i < framebufferCount_; ++i)
		{

		}
	}

private:
	std::vector<VkFramebuffer> framebuffers_;
	std::vector<VulkanImage*> images_;
	VkDevice device_;
	uint32_t framebufferCount_;
};

#endif