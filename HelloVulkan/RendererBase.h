#ifndef RENDERER_BASE
#define RENDERER_BASE

#define VK_NO_PROTOTYPES
#include "volk.h"

#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "RenderPassCreateInfo.h"
#include "UBO.h"

class RendererBase
{
public:
	explicit RendererBase(const VulkanDevice& vkDev, VulkanImage depthTexture);
	virtual ~RendererBase();
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) = 0;

	VulkanImage GetDepthTexture() const { return depthTexture_; }

	void SetUBO(const VulkanDevice& vkDev, uint32_t imageIndex, PerFrameUBO ubo)
	{
		UpdateUniformBuffer(vkDev.GetDevice(), imageIndex, &ubo, sizeof(PerFrameUBO));
	}

protected:
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

	// Pipeline & render pass (using DescriptorSets & pipeline state options)
	VkRenderPass renderPass_ = nullptr;
	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline graphicsPipeline_ = nullptr;

	// UBOs
	std::vector<VulkanBuffer> uniformBuffers_;

protected:
	void BeginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage);

	bool CreateUniformBuffers(VulkanDevice& vkDev, size_t uniformDataSize);

	bool CreateColorAndDepthRenderPass(
		VulkanDevice& device, 
		bool useDepth, 
		VkRenderPass* renderPass, 
		const RenderPassCreateInfo& ci, 
		VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM);

	bool CreateColorAndDepthFramebuffers(
		VulkanDevice& vkDev, 
		VkRenderPass renderPass, 
		VkImageView depthImageView, 
		std::vector<VkFramebuffer>& swapchainFramebuffers);

	bool CreateDescriptorPool(
		VulkanDevice& vkDev, 
		uint32_t uniformBufferCount, 
		uint32_t storageBufferCount, 
		uint32_t samplerCount, 
		VkDescriptorPool* descriptorPool);

	bool CreatePipelineLayout(VkDevice device, 
		VkDescriptorSetLayout dsLayout, 
		VkPipelineLayout* pipelineLayout);

	bool CreateGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass, 
		VkPipelineLayout pipelineLayout,
		const std::vector<const char*>& shaderFiles,
		VkPipeline* pipeline,
		bool hasVertexBuffer = false,
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST /* defaults to triangles*/,
		bool useDepth = true,
		bool useBlending = true,
		bool dynamicScissorState = false,
		int32_t customWidth = -1,
		int32_t customHeight = -1,
		uint32_t numPatchControlPoints = 0);

	// UBO
	void UpdateUniformBuffer(
		VkDevice device,
		uint32_t currentImage,
		const void* data,
		const size_t dataSize);
};

#endif