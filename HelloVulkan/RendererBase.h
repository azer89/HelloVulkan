#ifndef RENDERER_BASE
#define RENDERER_BASE

#include "volk.h"

#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "UBO.h"

#include <string>

/*enum RenderPassBit : uint8_t
{
	// Clear color attachment
	OffScreenColorClear = 0x01,

	// Transition color attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	// for the next onscreen render pass
	OffScreenColorShaderReadOnly = 0x02,

	// Clear swapchain color attachment
	OnScreenColorClear = 0x04,

	// Clear depth attachment
	OnScreenDepthClear = 0x08,

	// Present swapchain color attachment as VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	OnScreenColorPresent = 0x10
};*/

class RendererBase
{
public:
	explicit RendererBase(
		const VulkanDevice& vkDev, 
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage = nullptr,
		uint8_t renderPassBit = 0u);
	virtual ~RendererBase();

	// Insert Vulkan commands into the command buffer.
	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) = 0;

	void SetPerFrameUBO(const VulkanDevice& vkDev, uint32_t imageIndex, PerFrameUBO ubo)
	{
		UpdateUniformBuffer(vkDev.GetDevice(), perFrameUBOs_[imageIndex], &ubo, sizeof(PerFrameUBO));
	}

protected:
	VkDevice device_ = nullptr;

	uint32_t framebufferWidth_ = 0;
	uint32_t framebufferHeight_ = 0;

	// Depth buffer
	VulkanImage* depthImage_;

	// Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
	VkDescriptorSetLayout descriptorSetLayout_ = nullptr;
	VkDescriptorPool descriptorPool_ = nullptr;

	// Framebuffers (one for each command buffer)
	std::vector<VkFramebuffer> swapchainFramebuffers_;

	// Render pass
	VulkanRenderPass renderPass_;
	//VkRenderPass renderPass_ = nullptr;
	// Information about the renderpass
	//uint8_t renderPassBit_;

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
		return offscreenColorImage_ != nullptr;
	}

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

	//void BeginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage);
	//void BeginRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);

	/*void CreateOffScreenRenderPass(
		VulkanDevice& device,
		VkRenderPass* renderPass,
		uint8_t flag = 0u);

	void CreateOnScreenRenderPass(
		VulkanDevice& device, 
		VkRenderPass* renderPass, 
		uint8_t flag = 0u);
	*/
	void CreateOffScreenFramebuffer(
		VulkanDevice& vkDev,
		VulkanRenderPass renderPass,
		VkImageView outputImageView,
		VkImageView depthImageView,
		VkFramebuffer& framebuffers);

	void CreateOnScreenFramebuffers(
		VulkanDevice& vkDev, 
		VulkanRenderPass renderPass, 
		VkImageView depthImageView);

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
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST /* defaults to triangles*/,
		bool useDepth = true,
		bool useBlending = true,
		bool dynamicScissorState = false,
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