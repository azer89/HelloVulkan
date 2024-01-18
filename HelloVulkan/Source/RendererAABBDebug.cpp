#include "RendererAABBDebug.h"
#include "Configs.h"

#include <array>

RendererAABBDebug::RendererAABBDebug(
	VulkanDevice& vkDev,
	ClusterForwardBuffers* cfBuffers,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	RendererBase(vkDev, true), // Offscreen rendering
	shouldRender_(true),
	cfBuffers_(cfBuffers)
{
	CreateUniformBuffers(vkDev, perFrameUBOs_, sizeof(PerFrameUBO));
	// Create UBO Buffer
	invViewBuffer_.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		sizeof(InverseViewUBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkSampleCountFlagBits multisampleCount = offscreenColorImage->multisampleCount_;
	renderPass_.CreateOffScreenRenderPass(vkDev, renderBit, multisampleCount);

	framebuffer_.Create(
		vkDev,
		renderPass_.GetHandle(),
		{
			offscreenColorImage,
			depthImage
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
			AppConfig::ShaderFolder + "AABBRender.vert",
			AppConfig::ShaderFolder + "AABBRender.frag",
		},
		&graphicsPipeline_,
		false, // has no vertex buffer
		multisampleCount // For multisampling
		);
}

RendererAABBDebug::~RendererAABBDebug()
{
	invViewBuffer_.Destroy(device_);
}

void RendererAABBDebug::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
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
		36, // Draw a cube
		ClusterForwardConfig::sliceCountX * ClusterForwardConfig::sliceCountY * 12,
		0,
		0);

	vkCmdEndRenderPass(commandBuffer);
}

void RendererAABBDebug::CreateDescriptorLayoutAndSet(VulkanDevice& vkDev)
{
	const std::array<VkDescriptorSetLayoutBinding, 3> bindings =
	{
		DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
		DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
		DescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
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
		const VkDescriptorBufferInfo bufferInfo1 = {
			.buffer = perFrameUBOs_[i].buffer_,
			.offset = 0,
			.range = sizeof(PerFrameUBO)
		};
		const VkDescriptorBufferInfo bufferInfo2 = {
			.buffer = invViewBuffer_.buffer_,
			.offset = 0,
			.range = sizeof(InverseViewUBO)
		};
		const VkDescriptorBufferInfo bufferInfo3 = {
			.buffer = cfBuffers_->aabbBuffer_.buffer_,
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};

		const std::array<VkWriteDescriptorSet, 3> descriptorWrites = {
			BufferWriteDescriptorSet(
				descriptorSets_[i],
				&bufferInfo1,
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			BufferWriteDescriptorSet(
				descriptorSets_[i],
				&bufferInfo2,
				1,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			BufferWriteDescriptorSet(
				descriptorSets_[i],
				&bufferInfo3,
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		};

		vkUpdateDescriptorSets(
			vkDev.GetDevice(),
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);
	}
}