#ifndef PIPELINE_BASE
#define PIPELINE_BASE

#include "volk.h"

#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "UBO.h"

#include <string>

/*
This mainly encapsulates a graphics pipeline, framebuffers, and a render pass.
 */
class PipelineBase
{
public:
	explicit PipelineBase(
		const VulkanDevice& vkDev,
		bool isOffscreen = false);
	virtual ~PipelineBase();

	// If the window is resized
	virtual void OnWindowResized(VulkanDevice& vkDev);

	virtual void FillCommandBuffer(
		VulkanDevice& vkDev, 
		VkCommandBuffer commandBuffer, 
		size_t currentImage) = 0;

	void SetPerFrameUBO(const VulkanDevice& vkDev, uint32_t imageIndex, PerFrameUBO ubo)
	{
		UpdateUniformBuffer(vkDev.GetDevice(), perFrameUBOs_[imageIndex], &ubo, sizeof(PerFrameUBO));
	}

protected:
	VkDevice device_ = nullptr;

	// Offscreen rendering
	bool isOffscreen_;

	VulkanFramebuffer framebuffer_;

	// Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
	VkDescriptorSetLayout descriptorSetLayout_ = nullptr;
	VkDescriptorPool descriptorPool_ = nullptr;

	// Render pass
	VulkanRenderPass renderPass_;

	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline graphicsPipeline_ = nullptr;

	// PerFrameUBO
	std::vector<VulkanBuffer> perFrameUBOs_;

protected:
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

	void CreateDescriptorPool(
		VulkanDevice& vkDev,
		uint32_t uniformBufferCount,
		uint32_t storageBufferCount,
		uint32_t samplerCount,
		uint32_t setCountPerSwapchain,
		VkDescriptorPool* descriptorPool,
		VkDescriptorPoolCreateFlags flags = 0);

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