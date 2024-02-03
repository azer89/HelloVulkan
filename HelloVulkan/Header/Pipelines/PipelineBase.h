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
Note that a pipeline can be either graphics or compute.
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

	virtual void FillCommandBuffer(
		VulkanDevice& vkDev, 
		VkCommandBuffer commandBuffer) = 0;

	void SetCameraUBO(VulkanDevice& vkDev, uint32_t imageIndex, CameraUBO ubo)
	{
		cameraUBOBuffers_[imageIndex].UploadBufferData(vkDev, 0, &ubo, sizeof(CameraUBO));
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
	// TODO move to VulkanBuffer
	void CreateUniformBuffers(
		VulkanDevice& vkDev,
		std::vector<VulkanBuffer>& buffers,
		size_t uniformDataSize);

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