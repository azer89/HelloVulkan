#ifndef PIPELINE_BASE
#define PIPELINE_BASE

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanDescriptorManager.h"
#include "VulkanSpecialization.h"
#include "PipelineConfig.h"
#include "UIData.h"
#include "UBOs.h"

#include <string>

/*
This mainly encapsulates a graphics pipeline, framebuffers, and a render pass.
A pipeline can be either
	* Offscreen graphics (draw to an image)
	* Onscreen graphics (draw to a swapchain image)
	* Compute
	* Raytracing
 */
class PipelineBase
{
public:
	explicit PipelineBase(
		const VulkanContext& ctx,
		const PipelineConfig& config);
	virtual ~PipelineBase();

	// If the window is resized
	virtual void OnWindowResized(VulkanContext& ctx);

	// TODO Maybe rename to RecordCommandBuffer
	virtual void FillCommandBuffer(
		VulkanContext& ctx, 
		VkCommandBuffer commandBuffer) = 0;

	virtual void UpdateFromUIData(VulkanContext& ctx, UIData& uiData)
	{
	}

	virtual void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo)
	{
		const uint32_t frameIndex = ctx.GetFrameIndex();
		cameraUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(CameraUBO));
	}

protected:
	VkDevice device_{};
	PipelineConfig config_;

	// Keep this as a vector because it can be empty when not used
	std::vector<VulkanBuffer> cameraUBOBuffers_;

	VulkanFramebuffer framebuffer_;
	VulkanDescriptorManager descriptorManager_;
	VulkanRenderPass renderPass_;
	VulkanSpecialization specializationConstants_;
	VkPipelineLayout pipelineLayout_{};
	VkPipeline pipeline_{};

	// Multiple render target
	std::vector<VkPipelineColorBlendAttachmentState> overridingColorBlendAttachments{};

protected:
	bool IsOffscreen() const
	{
		return config_.type_ == PipelineType::GraphicsOffScreen;
	}

	void AddOverridingColorBlendAttachment(
		VkColorComponentFlags colorWriteMask,
		VkBool32 blendEnable);

	void BindPipeline(VulkanContext& ctx, VkCommandBuffer commandBuffer);

	void CreatePipelineLayout(VulkanContext& ctx,
		VkDescriptorSetLayout dsLayout,
		VkPipelineLayout* pipelineLayout,
		uint32_t pushConstantSize = 0,
		VkShaderStageFlags pushConstantShaderStage = 0);

	void CreateGraphicsPipeline(
		VulkanContext& ctx,
		VkRenderPass renderPass, 
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline);

	void CreateComputePipeline(
		VulkanContext& ctx,
		const std::string& shaderFile);
};

#endif