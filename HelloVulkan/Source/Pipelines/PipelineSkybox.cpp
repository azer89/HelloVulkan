#include "PipelineSkybox.h"
#include "VulkanUtility.h"
#include "Configs.h"
#include "UBO.h"

#include "glm/glm.hpp"

#include <cmath>
#include <array>

PipelineSkybox::PipelineSkybox(VulkanDevice& vkDev, 
	VulkanImage* envMap,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(vkDev,
		{
			.flags_ = PipelineFlags::GraphicsOffScreen,
			.msaaSamples_ = offscreenColorImage->multisampleCount_,
			.vertexBufferBind_ = false,
		}),
	envCubemap_(envMap)
{
	CreateUniformBuffers(vkDev, perFrameUBOs_, sizeof(PerFrameUBO));

	VkSampleCountFlagBits multisampleCount = VK_SAMPLE_COUNT_1_BIT;

	// Note that this pipeline is offscreen rendering
	multisampleCount = offscreenColorImage->multisampleCount_;
	renderPass_.CreateOffScreenRenderPass(vkDev, renderBit, multisampleCount);
	framebuffer_.Create(
		vkDev,
		renderPass_.GetHandle(),
		{
			offscreenColorImage,
			depthImage
		},
		IsOffscreen()
	);

	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 1u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.swapchainCount_ = static_cast<uint32_t>(vkDev.GetSwapchainImageCount()),
			.setCountPerSwapchain_ = 1u,
		});
	CreateDescriptorLayoutAndSet(vkDev);
	
	CreatePipelineLayout(vkDev.GetDevice(), descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Cube.vert",
			AppConfig::ShaderFolder + "Cube.frag",
		},
		&pipeline_//,
		//false, // has no vertex buffer
		//multisampleCount // For multisampling
		); 
}

PipelineSkybox::~PipelineSkybox()
{
}

void PipelineSkybox::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(vkDev, commandBuffer);

	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		pipelineLayout_, 
		0, 
		1, 
		&descriptorSets_[swapchainImageIndex], 
		0, 
		nullptr);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void PipelineSkybox::CreateDescriptorLayoutAndSet(VulkanDevice& vkDev)
{
	descriptor_.CreateLayout(vkDev,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT,
			.bindingCount_ = 1
		},
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		}
	});

	auto swapChainImageSize = vkDev.GetSwapchainImageCount();
	descriptorSets_.resize(swapChainImageSize);

	VkDescriptorImageInfo imageInfo = envCubemap_->GetDescriptorImageInfo();

	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		VkDescriptorBufferInfo bufferInfo = { perFrameUBOs_[i].buffer_, 0, sizeof(PerFrameUBO) };

		descriptor_.CreateSet(
			vkDev,
			{
				{.bufferInfoPtr_ = &bufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
				{.imageInfoPtr_ = &imageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }
			},
			&(descriptorSets_[i]));
	}
}