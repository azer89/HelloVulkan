#include "PipelineLight.h"
#include "VulkanUtility.h"

#include "Configs.h"

#include <array>

PipelineLight::PipelineLight(
	VulkanDevice& vkDev,
	Lights* lights,
	VulkanImage* depthImage, 
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(vkDev, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = offscreenColorImage->multisampleCount_
		}
	), // Offscreen rendering
	lights_(lights),
	shouldRender_(true)
{
	CreateUniformBuffers(vkDev, perFrameUBOs_, sizeof(PerFrameUBO));

	renderPass_.CreateOffScreenRenderPass(vkDev, renderBit, config_.msaaSamples_);

	framebuffer_.Create(
		vkDev,
		renderPass_.GetHandle(),
		{
			offscreenColorImage,
			depthImage
		},
		IsOffscreen()
	);

	SetupDescriptor(vkDev);

	CreatePipelineLayout(vkDev.GetDevice(), descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "LightCircle.vert",
			AppConfig::ShaderFolder + "LightCircle.frag",
		},
		&pipeline_
		);
}

PipelineLight::~PipelineLight()
{
}

void PipelineLight::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
	if (!shouldRender_)
	{
		return;
	}

	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(vkDev, commandBuffer);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0,
		1,
		&descriptorSets_[currentImage],
		0,
		nullptr);

	vkCmdDraw(
		commandBuffer, 
		6, // Draw a quad
		lights_->GetLightCount(), 
		0, 
		0);

	vkCmdEndRenderPass(commandBuffer);
}

void PipelineLight::SetupDescriptor(VulkanDevice& vkDev)
{
	// Pool
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 1u,
			.ssboCount_ = 1u,
			.samplerCount_ = 0u,
			.swapchainCount_ = static_cast<uint32_t>(vkDev.GetSwapchainImageCount()),
			.setCountPerSwapchain_ = 1u,
		});

	// Layout
	descriptor_.CreateLayout(vkDev,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		},
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	size_t swapchainLength = vkDev.GetSwapchainImageCount();
	descriptorSets_.resize(swapchainLength);

	for (size_t i = 0; i < swapchainLength; ++i)
	{
		VkDescriptorBufferInfo bufferInfo1 = {.buffer = perFrameUBOs_[i].buffer_, .offset = 0, .range = sizeof(PerFrameUBO)};
		VkDescriptorBufferInfo bufferInfo2 = {.buffer = lights_->GetSSBOBuffer(), .offset = 0, .range = lights_->GetSSBOSize()};

		descriptor_.CreateSet(
			vkDev, 
			{
				{.bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
			}, 
			&(descriptorSets_[i]));
	}
}