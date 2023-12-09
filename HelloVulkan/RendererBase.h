#ifndef RENDERER_BASE
#define RENDERER_BASE

#define VK_NO_PROTOTYPES
#include "volk.h"

#include "VulkanDevice.h"
#include "VulkanImage.h"

class RendererBase
{
public:
	explicit RendererBase(const VulkanDevice& vkDev, VulkanImage depthTexture);
	virtual ~RendererBase();
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) = 0;

	inline VulkanImage GetDepthTexture() const { return depthTexture_; }

protected:
	void BeginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage);
	bool CreateUniformBuffers(VulkanDevice& vkDev, size_t uniformDataSize);

	bool CreateUniformBuffer(VulkanDevice& vkDev,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory,
		VkDeviceSize bufferSize);
	bool CreateBuffer(VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory);
	uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkDevice device_ = nullptr;

	uint32_t framebufferWidth_ = 0;
	uint32_t framebufferHeight_ = 0;

	// Depth buffer
	VulkanImage depthTexture_;

	// Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
	VkDescriptorSetLayout descriptorSetLayout_ = nullptr;
	VkDescriptorPool descriptorPool_ = nullptr;
	std::vector<VkDescriptorSet> descriptorSets_;

	// Framebuffers (one for each command buffer)
	std::vector<VkFramebuffer> swapchainFramebuffers_;

	// 4. Pipeline & render pass (using DescriptorSets & pipeline state options)
	VkRenderPass renderPass_ = nullptr;
	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline graphicsPipeline_ = nullptr;

	// 5. Uniform buffer
	std::vector<VkBuffer> uniformBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;
};

#endif