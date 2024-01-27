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
	PipelineBase(vkDev, PipelineFlags::GraphicsOffScreen), // Offscreen rendering
	lights_(lights),
	shouldRender_(true)
{
	CreateUniformBuffers(vkDev, perFrameUBOs_, sizeof(PerFrameUBO));

	VkSampleCountFlagBits multisampleCount = offscreenColorImage->multisampleCount_;
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

	/*CreateDescriptorPool(
		vkDev,
		1, // UBO
		1, // SSBO
		0, // Texture
		1, // One set per swapchain
		&descriptorPool_);*/
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 1u,
			.ssboCount_ = 1u,
			.samplerCount_ = 0u,
			.swapchainCount_ = static_cast<uint32_t>(vkDev.GetSwapchainImageCount()),
			.setCountPerSwapchain_ = 1u,
		});
	CreateDescriptorLayoutAndSet(vkDev);

	CreatePipelineLayout(vkDev.GetDevice(), descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "LightCircle.vert",
			AppConfig::ShaderFolder + "LightCircle.frag",
		},
		&pipeline_,
		false, // has no vertex buffer
		multisampleCount // For multisampling
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

void PipelineLight::CreateDescriptorLayoutAndSet(VulkanDevice& vkDev)
{
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

	/*const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
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

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout_));*/

	size_t swapchainLength = vkDev.GetSwapchainImageCount();

	/*std::vector<VkDescriptorSetLayout> layouts(scImageCount, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(scImageCount),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(scImageCount);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));
	*/

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

		/*const std::array<VkWriteDescriptorSet, 2> descriptorWrites = {
			BufferWriteDescriptorSet(
				descriptorSets_[i],
				&bufferInfo1,
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			BufferWriteDescriptorSet(
				descriptorSets_[i],
				&bufferInfo2,
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		};

		vkUpdateDescriptorSets(
			vkDev.GetDevice(), 
			static_cast<uint32_t>(descriptorWrites.size()), 
			descriptorWrites.data(), 
			0, 
			nullptr);*/
	}
}