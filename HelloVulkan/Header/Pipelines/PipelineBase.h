#ifndef PIPELINE_BASE
#define PIPELINE_BASE

#include "volk.h"

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanDescriptor.h"
#include "PipelineConfig.h"
#include "UBO.h"

#include <string>

/*
This mainly encapsulates a graphics pipeline, framebuffers, and a render pass.
A pipeline can be either
	* Offscreen graphics (draw to an image)
	* Onscreen graphics (draw to a swapchain image)
	* Compute
 */
class PipelineBase
{
public:
	explicit PipelineBase(
		const VulkanContext& vkDev,
		PipelineConfig config);
	virtual ~PipelineBase();

	// If the window is resized
	virtual void OnWindowResized(VulkanContext& vkDev);

	// TODO Maybe rename to RecordCommandBuffer
	virtual void FillCommandBuffer(
		VulkanContext& vkDev, 
		VkCommandBuffer commandBuffer) = 0;

	void SetCameraUBO(VulkanContext& vkDev, CameraUBO ubo)
	{
		uint32_t frameIndex = vkDev.GetFrameIndex();
		cameraUBOBuffers_[frameIndex].UploadBufferData(vkDev, 0, &ubo, sizeof(CameraUBO));
	}

protected:
	VkDevice device_ = nullptr;
	PipelineConfig config_;

	// Keep this as a vector because it can be empty when not used
	std::vector<VulkanBuffer> cameraUBOBuffers_;

	VulkanFramebuffer framebuffer_;
	VulkanDescriptor descriptor_;
	VulkanRenderPass renderPass_;
	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline pipeline_ = nullptr;

protected:
	bool IsOffscreen() const
	{
		return config_.type_ == PipelineType::GraphicsOffScreen;
	}

	void BindPipeline(VulkanContext& vkDev, VkCommandBuffer commandBuffer);

	// UBO
	void CreateMultipleUniformBuffers(
		VulkanContext& vkDev,
		std::vector<VulkanBuffer>& buffers,
		uint32_t dataSize,
		size_t bufferCount);

	void CreatePipelineLayout(VulkanContext& vkDev,
		VkDescriptorSetLayout dsLayout, 
		VkPipelineLayout* pipelineLayout,
		const std::vector<VkPushConstantRange>& pushConstantRanges = {});

	void CreateGraphicsPipeline(
		VulkanContext& vkDev,
		VkRenderPass renderPass, 
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline);

	void CreateComputePipeline(
		VulkanContext& vkDev,
		const std::string& shaderFile);
};

#endif