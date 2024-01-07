#ifndef RENDERER_BASE
#define RENDERER_BASE

#include "volk.h"

#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "UBO.h"

#include <string>

class RendererBase
{
public:
	explicit RendererBase(
		const VulkanDevice& vkDev, 
		// TODO refactor pointers of VulkanImage as render pass attachments
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage = nullptr,
		uint8_t renderPassBit = 0u);
	virtual ~RendererBase();

	virtual void FillCommandBuffer(
		VulkanDevice& vkDev, 
		VkCommandBuffer commandBuffer, 
		size_t currentImage) = 0;

	void SetPerFrameUBO(const VulkanDevice& vkDev, uint32_t imageIndex, PerFrameUBO ubo)
	{
		UpdateUniformBuffer(vkDev.GetDevice(), perFrameUBOs_[imageIndex], &ubo, sizeof(PerFrameUBO));
	}

	// If the window is resized
	virtual void OnWindowResized(VulkanDevice& vkDev);

protected:
	VkDevice device_ = nullptr;

	// Depth buffer
	VulkanImage* depthImage_;

	// Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
	VkDescriptorSetLayout descriptorSetLayout_ = nullptr;
	VkDescriptorPool descriptorPool_ = nullptr;

	// Framebuffers (one for each command buffer)
	std::vector<VkFramebuffer> swapchainFramebuffers_;

	// Render pass
	VulkanRenderPass renderPass_;

	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline graphicsPipeline_ = nullptr;

	// PerFrameUBO
	std::vector<VulkanBuffer> perFrameUBOs_;

	// Offscreen rendering
	VulkanImage* offscreenColorImage_;
	VkFramebuffer offscreenFramebuffer_;

protected:
	inline bool IsOffScreen()
	{
		return offscreenColorImage_ != nullptr ||
			offscreenFramebuffer_ != nullptr;
	}

	void BindPipeline(VulkanDevice& vkDev, VkCommandBuffer commandBuffer);

	// UBO
	void CreateUniformBuffers(
		VulkanDevice& vkDev,
		std::vector<VulkanBuffer>& buffers,
		size_t uniformDataSize);

	// UBO
	void UpdateUniformBuffer(
		VkDevice device,
		VulkanBuffer& buffer,
		const void* data,
		const size_t dataSize);

	// TODO consolidate offscreen/onscreen into a single function
	// Attach an array of image views to a framebuffer
	void CreateSingleFramebuffer(
		VulkanDevice& vkDev,
		VulkanRenderPass renderPass,
		const std::vector<VkImageView>& imageViews,
		VkFramebuffer& framebuffer);

	// Attach a swapchain image and depth image
	void CreateSwapchainFramebuffers(
		VulkanDevice& vkDev, 
		VulkanRenderPass renderPass, 
		VkImageView depthImageView = nullptr);
	
	void DestroySwapchainFramebuffers();

	void CreateDescriptorPool(
		VulkanDevice& vkDev, 
		uint32_t uniformBufferCount, 
		uint32_t storageBufferCount, 
		uint32_t samplerCount, 
		uint32_t setCountPerSwapchain,
		VkDescriptorPool* descriptorPool);

	void CreatePipelineLayout(VkDevice device,
		VkDescriptorSetLayout dsLayout, 
		VkPipelineLayout* pipelineLayout,
		const std::vector<VkPushConstantRange>& pushConstantRanges = {});

	void CreateGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass, 
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline,
		bool hasVertexBuffer = false,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT,
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST /* defaults to triangles*/,
		bool useDepth = true,
		bool useBlending = true,
		uint32_t numPatchControlPoints = 0);

	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(
		uint32_t binding,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		uint32_t descriptorCount = 1);

	VkWriteDescriptorSet BufferWriteDescriptorSet(
		VkDescriptorSet ds,
		const VkDescriptorBufferInfo* bi,
		uint32_t bindIdx,
		VkDescriptorType dType);

	VkWriteDescriptorSet ImageWriteDescriptorSet(
		VkDescriptorSet ds,
		const VkDescriptorImageInfo* ii,
		uint32_t bindIdx);
};

#endif