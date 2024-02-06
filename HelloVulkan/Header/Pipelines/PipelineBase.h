#ifndef PIPELINE_BASE
#define PIPELINE_BASE

#include "volk.h"

#include "VulkanDevice.h"
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
	* Onscreen graphics (draw to an image)
	* Offscreen graphics (draw to a swapchain image)
	* Compute
 */
class PipelineBase
{
public:
	explicit PipelineBase(
		const VulkanDevice& vkDev,
		PipelineConfig config);
	virtual ~PipelineBase();

	// If the window is resized
	virtual void OnWindowResized(VulkanDevice& vkDev);

	// TODO Maybe rename to RecordCommandBuffer
	virtual void FillCommandBuffer(
		VulkanDevice& vkDev, 
		VkCommandBuffer commandBuffer) = 0;

	void SetCameraUBO(VulkanDevice& vkDev, CameraUBO ubo)
	{
		uint32_t frameIndex = vkDev.GetFrameIndex();
		cameraUBOBuffers_[frameIndex].UploadBufferData(vkDev, 0, &ubo, sizeof(CameraUBO));
	}

protected:
	VkDevice device_ = nullptr;
	PipelineConfig config_;
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

	void BindPipeline(VulkanDevice& vkDev, VkCommandBuffer commandBuffer);

	// UBO
	void CreateMultipleUniformBuffers(
		VulkanDevice& vkDev,
		std::vector<VulkanBuffer>& buffers,
		uint32_t dataSize,
		size_t bufferCount);

	void CreatePipelineLayout(VulkanDevice& vkDev,
		VkDescriptorSetLayout dsLayout, 
		VkPipelineLayout* pipelineLayout,
		const std::vector<VkPushConstantRange>& pushConstantRanges = {});

	void CreateGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass, 
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline);

	void CreateComputePipeline(
		VulkanDevice& vkDev,
		const std::string& shaderFile);
};

#endif