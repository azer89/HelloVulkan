#ifndef RENDERER_BASE
#define RENDERER_BASE

#include "volk.h"

#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "UBO.h"

#include <string>

struct RenderPassCreateInfo final
{
	bool clearColor_ = false;
	bool clearDepth_ = false;
	//uint8_t flags_ = 0;
	bool renderFirst_ = false;
	bool renderLast_ = false;
};

/*enum eRenderPassBit : uint8_t
{
	eRenderPassBit_First = 0x01,
	eRenderPassBit_Last = 0x02,
};*/

class RendererBase
{
public:
	explicit RendererBase(const VulkanDevice& vkDev, VulkanImage* depthImage);
	virtual ~RendererBase();
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) = 0;

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

	// Pipeline & render pass (using DescriptorSets & pipeline state options)
	VkRenderPass renderPass_ = nullptr;
	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline graphicsPipeline_ = nullptr;

	// PerFrameUBO
	std::vector<VulkanBuffer> perFrameUBOs_;

protected:
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

	void BeginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage);

	void CreateColorAndDepthRenderPass(
		VulkanDevice& device, 
		bool useDepth, 
		VkRenderPass* renderPass, 
		const RenderPassCreateInfo& ci, 
		VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM);

	void CreateColorAndDepthFramebuffers(
		VulkanDevice& vkDev, 
		VkRenderPass renderPass, 
		VkImageView depthImageView, 
		std::vector<VkFramebuffer>& swapchainFramebuffers);

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