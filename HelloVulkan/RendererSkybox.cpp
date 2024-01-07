#include "RendererSkybox.h"
#include "VulkanUtility.h"
#include "AppSettings.h"
#include "UBO.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include <cmath>
#include <array>

RendererSkybox::RendererSkybox(VulkanDevice& vkDev, 
	VulkanImage* envMap,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	RendererBase(vkDev, depthImage, offscreenColorImage, renderBit),
	specularMap_(envMap)
{
	CreateUniformBuffers(vkDev, perFrameUBOs_, sizeof(PerFrameUBO));

	VkSampleCountFlagBits multisampleCount = VK_SAMPLE_COUNT_1_BIT;

	// Note that this pipeline is offscreen rendering
	multisampleCount = offscreenColorImage_->multisampleCount_;
	renderPass_.CreateOffScreenRenderPass(vkDev, renderBit, multisampleCount);
	CreateSingleFramebuffer(
		vkDev,
		renderPass_,
		{
			offscreenColorImage_->imageView_,
			depthImage_->imageView_
		},
		offscreenFramebuffer_);
	
	CreateDescriptorPool(
		vkDev, 
		1, // uniform
		0, // SSBO
		1, // Texture
		1, // One set per swapchain
		&descriptorPool_);
	CreateDescriptorLayoutAndSet(vkDev);
	
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);

	CreateGraphicsPipeline(vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppSettings::ShaderFolder + "Cube.vert",
			AppSettings::ShaderFolder + "Cube.frag",
		},
		&graphicsPipeline_,
		false, // has no vertex buffer
		multisampleCount // For multisampling
		); 
}

RendererSkybox::~RendererSkybox()
{
}

void RendererSkybox::OnWindowResized(VulkanDevice& vkDev)
{
	vkDestroyFramebuffer(vkDev.GetDevice(), offscreenFramebuffer_, nullptr);
	CreateSingleFramebuffer(
		vkDev,
		renderPass_,
		{
			offscreenColorImage_->imageView_,
			depthImage_->imageView_
		},
		offscreenFramebuffer_);
}

void RendererSkybox::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(
		vkDev,
		commandBuffer, 
		IsOffScreen() ?
			offscreenFramebuffer_ :
			swapchainFramebuffers_[swapchainImageIndex]);

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

void RendererSkybox::CreateDescriptorLayoutAndSet(VulkanDevice& vkDev)
{
	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		DescriptorSetLayoutBinding(
			0, 
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			VK_SHADER_STAGE_VERTEX_BIT),
		DescriptorSetLayoutBinding(
			1, 
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
			VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(
		vkDev.GetDevice(), 
		&layoutInfo, 
		nullptr, 
		&descriptorSetLayout_));

	auto swapChainImageSize = vkDev.GetSwapchainImageCount();

	std::vector<VkDescriptorSetLayout> layouts(swapChainImageSize, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImageSize),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(swapChainImageSize);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));

	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo = 
			{ perFrameUBOs_[i].buffer_, 0, sizeof(PerFrameUBO) };
		const VkDescriptorImageInfo  imageInfo = 
		{
			specularMap_->defaultImageSampler_,
			specularMap_->imageView_,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		const std::array<VkWriteDescriptorSet, 2> descriptorWrites = {
			BufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			ImageWriteDescriptorSet(ds, &imageInfo,   1)
		};

		vkUpdateDescriptorSets(
			vkDev.GetDevice(), 
			static_cast<uint32_t>(descriptorWrites.size()), 
			descriptorWrites.data(), 
			0, 
			nullptr);
	}
}