#include "RendererBase.h"
#include "VulkanUtility.h"

RendererBase::RendererBase(const VulkanDevice& vkDev, VulkanImage depthTexture)
	: device_(vkDev.GetDevice())
	, framebufferWidth_(vkDev.GetFrameBufferWidth())
	, framebufferHeight_(vkDev.GetFrameBufferHeight())
	, depthTexture_(depthTexture)
{
}

RendererBase::~RendererBase()
{
	for (auto buf : uniformBuffers_)
		vkDestroyBuffer(device_, buf, nullptr);

	for (auto mem : uniformBuffersMemory_)
		vkFreeMemory(device_, mem, nullptr);

	vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
	vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

	for (auto framebuffer : swapchainFramebuffers_)
		vkDestroyFramebuffer(device_, framebuffer, nullptr);

	vkDestroyRenderPass(device_, renderPass_, nullptr);
	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
}

void RendererBase::BeginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage)
{
	const VkRect2D screenRect = {
		.offset = { 0, 0 },
		.extent = {.width = framebufferWidth_, .height = framebufferHeight_ }
	};

	const VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = renderPass_,
		.framebuffer = swapchainFramebuffers_[currentImage],
		.renderArea = screenRect
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &descriptorSets_[currentImage], 0, nullptr);
}

bool RendererBase::CreateUniformBuffers(VulkanDevice& vkDev, size_t uniformDataSize)
{
	auto swapChainImageSize = vkDev.GetSwapChainImageSize();
	uniformBuffers_.resize(swapChainImageSize);
	uniformBuffersMemory_.resize(swapChainImageSize);
	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		if (!CreateUniformBuffer(vkDev, uniformBuffers_[i], uniformBuffersMemory_[i], uniformDataSize))
		{
			printf("Cannot create uniform buffer\n");
			fflush(stdout);
			return false;
		}
	}
	return true;
}

bool RendererBase::CreateUniformBuffer(VulkanDevice& vkDev,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory,
	VkDeviceSize bufferSize)
{
	return CreateBuffer(vkDev.GetDevice(), 
		vkDev.GetPhysicalDevice(), 
		bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		buffer, 
		bufferMemory);
}

bool RendererBase::CreateBuffer(VkDevice device,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory)
{
	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));

	vkBindBufferMemory(device, buffer, bufferMemory, 0);

	return true;
}

uint32_t RendererBase::FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}