#include "RendererLight.h"
#include "VulkanUtility.h"

#include "Configs.h"

#include <array>

RendererLight::RendererLight(
	VulkanDevice& vkDev,
	Lights* lights,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	RendererBase(vkDev, true), // Offscreen rendering
	lights_(lights)
{
	CreateUniformBuffers(vkDev, perFrameUBOs_, sizeof(PerFrameUBO));

	VkSampleCountFlagBits multisampleCount = offscreenColorImage->multisampleCount_;
	renderPass_.CreateOffScreenRenderPass(vkDev, renderBit, multisampleCount);

	framebuffer_.Create(
		vkDev,
		renderPass_.GetHandle(),
		{
			offscreenColorImage
		},
		isOffscreen_
	);

	CreateDescriptorPool(
		vkDev,
		1, // UBO
		1, // SSBO
		0, // Texture
		1, // One set per swapchain
		&descriptorPool_);
	CreateDescriptorLayoutAndSet(vkDev);

	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);

	CreateGraphicsPipeline(vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppSettings::ShaderFolder + "LightCircle.vert",
			AppSettings::ShaderFolder + "LightCircle.frag",
		},
		&graphicsPipeline_,
		false, // has no vertex buffer
		multisampleCount // For multisampling
		);
}

RendererLight::~RendererLight()
{

}

void RendererLight::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{

}

void RendererLight::CreateDescriptorLayoutAndSet(VulkanDevice& vkDev)
{
	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
		DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout_));

	size_t scImageCount = vkDev.GetSwapchainImageCount();

	std::vector<VkDescriptorSetLayout> layouts(scImageCount, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(scImageCount),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(scImageCount);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));

	for (size_t i = 0; i < scImageCount; ++i)
	{
		const VkDescriptorBufferInfo bufferInfo = {
			.buffer = perFrameUBOs_[i].buffer_,
			.offset = 0,
			.range = sizeof(PerFrameUBO)
		};
		const VkDescriptorBufferInfo bufferInfo2 = {
			.buffer = lights_->GetSSBOBuffer(),
			.offset = 0,
			.range = lights_->GetSSBOSize()
		};

		// TODO the code below can be simplified
		const std::array<VkWriteDescriptorSet, 2> descriptorWrites = {
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorSets_[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo
			},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorSets_[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bufferInfo2
			},
		};

		vkUpdateDescriptorSets(
			vkDev.GetDevice(), 
			static_cast<uint32_t>(descriptorWrites.size()), 
			descriptorWrites.data(), 
			0, 
			nullptr);
	}
}