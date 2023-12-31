#include "RendererBase.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "Mesh.h"
#include "PipelineCreateInfo.h"

#include <array>

// Constructor
RendererBase::RendererBase(
	const VulkanDevice& vkDev, 
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderPassBit) :
	device_(vkDev.GetDevice()), 
	depthImage_(depthImage),
	offscreenColorImage_(offscreenColorImage)
{
}

// Destructor
RendererBase::~RendererBase()
{
	DestroySwapchainFramebuffers();

	for (auto uboBuffer : perFrameUBOs_)
	{
		uboBuffer.Destroy(device_);
	}

	vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
	vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
	vkDestroyFramebuffer(device_, offscreenFramebuffer_, nullptr);
	renderPass_.Destroy(device_);
	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
}

void RendererBase::CreateUniformBuffers(
	VulkanDevice& vkDev,
	std::vector<VulkanBuffer>& buffers,
	size_t uniformDataSize)
{
	auto swapChainImageSize = vkDev.GetSwapchainImageCount();
	buffers.resize(swapChainImageSize);
	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		bool res = buffers[i].CreateBuffer(
			vkDev.GetDevice(),
			vkDev.GetPhysicalDevice(),
			uniformDataSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		if (!res)
		{
			std::cerr << "Cannot create uniform buffer\n";
		}
	}
}

void RendererBase::BindPipeline(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

	VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)vkDev.GetFrameBufferWidth(),
		.height = (float)vkDev.GetFrameBufferHeight(),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { vkDev.GetFrameBufferWidth(), vkDev.GetFrameBufferHeight() };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void RendererBase::OnWindowResized(VulkanDevice& vkDev)
{
	// If this is offscreen renderer then it does not use swapchain framebuffers
	if (IsOffScreen())
	{
		return;
	}

	DestroySwapchainFramebuffers();
	VkImageView depthImageView = depthImage_ == nullptr ? nullptr : depthImage_->imageView_;
	CreateSwapchainFramebuffers(vkDev, renderPass_, depthImageView);
}

// Attach an array of image views to a framebuffer
void RendererBase::CreateSingleFramebuffer(
	VulkanDevice& vkDev,
	VulkanRenderPass renderPass,
	const std::vector<VkImageView>& imageViews,
	VkFramebuffer& framebuffer)
{
	const VkFramebufferCreateInfo framebufferInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass.GetHandle(),
		.attachmentCount = static_cast<uint32_t>(imageViews.size()),
		// TODO cache the attachments so that framebuffer recreation is easier
		.pAttachments = imageViews.data(),
		.width = vkDev.GetFrameBufferWidth(),
		.height = vkDev.GetFrameBufferHeight(),
		.layers = 1
	};

	VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &framebufferInfo, nullptr, &framebuffer));
}

void RendererBase::CreateSwapchainFramebuffers(
	VulkanDevice& vkDev,
	VulkanRenderPass renderPass,
	VkImageView depthImageView)
{
	size_t swapchainImageSize = vkDev.GetSwapchainImageCount();

	swapchainFramebuffers_.resize(swapchainImageSize);

	// Trick to put a swapchain image to the list of attachment.
	// Note that depthImageView can be nullptr.
	// TODO cache the attachments so that framebuffer recreation is easier
	std::vector<VkImageView> attachments = { nullptr, depthImageView };

	for (size_t i = 0; i < swapchainImageSize; i++)
	{
		attachments[0] = vkDev.GetSwapchainImageView(i);

		const VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = renderPass.GetHandle(),
			.attachmentCount = static_cast<uint32_t>((depthImageView == nullptr) ? 1 : 2),
			.pAttachments = attachments.data(),
			.width = vkDev.GetFrameBufferWidth(),
			.height = vkDev.GetFrameBufferHeight(),
			.layers = 1
		};

		VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers_[i]));
	}
}

void RendererBase::DestroySwapchainFramebuffers()
{
	for (auto framebuffer : swapchainFramebuffers_)
	{
		vkDestroyFramebuffer(device_, framebuffer, nullptr);
	}
}

void RendererBase::CreateDescriptorPool(
	VulkanDevice& vkDev,
	uint32_t uniformBufferCount,
	uint32_t storageBufferCount,
	uint32_t samplerCount,
	uint32_t setCountPerSwapchain,
	VkDescriptorPool* descriptorPool)
{
	const uint32_t imageCount = static_cast<uint32_t>(vkDev.GetSwapchainImageCount());

	std::vector<VkDescriptorPoolSize> poolSizes;

	if (uniformBufferCount)
		poolSizes.push_back(VkDescriptorPoolSize
			{ 
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				.descriptorCount = imageCount * uniformBufferCount 
			});

	if (storageBufferCount)
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
				.descriptorCount = imageCount * storageBufferCount 
			});

	if (samplerCount)
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
				.descriptorCount = imageCount * samplerCount 
			});

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = static_cast<uint32_t>(imageCount * setCountPerSwapchain),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data()
	};

	VK_CHECK(vkCreateDescriptorPool(vkDev.GetDevice(), &poolInfo, nullptr, descriptorPool));
}

void RendererBase::CreatePipelineLayout(
	VkDevice device, 
	VkDescriptorSetLayout dsLayout, 
	VkPipelineLayout* pipelineLayout,
	const std::vector<VkPushConstantRange>& pushConstantRanges)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	if (pushConstantRanges.size() > 0)
	{
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	}

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout));
}

void RendererBase::CreateGraphicsPipeline(
	VulkanDevice& vkDev,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<std::string>& shaderFiles,
	VkPipeline* pipeline,
	bool hasVertexBuffer,
	VkSampleCountFlagBits msaaSamples,
	VkPrimitiveTopology topology,
	bool useDepth,
	bool useBlending,
	uint32_t numPatchControlPoints)
{
	std::vector<VulkanShader> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize(shaderFiles.size());
	shaderModules.resize(shaderFiles.size());

	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i].c_str();
		VK_CHECK(shaderModules[i].Create(vkDev.GetDevice(), file));
		VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Pipeline create info
	PipelineCreateInfo pInfo(vkDev);

	std::vector<VkVertexInputBindingDescription> bindingDescriptions = 
		VertexData::GetBindingDescriptions();
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = 
		VertexData::GetAttributeDescriptions();

	if (hasVertexBuffer)
	{
		pInfo.vertexInputInfo.vertexAttributeDescriptionCount = 
			static_cast<uint32_t>(attributeDescriptions.size());
		pInfo.vertexInputInfo.vertexBindingDescriptionCount = 
			static_cast<uint32_t>(bindingDescriptions.size());
		pInfo.vertexInputInfo.pVertexAttributeDescriptions = 
			attributeDescriptions.data();
		pInfo.vertexInputInfo.pVertexBindingDescriptions = 
			bindingDescriptions.data();
	}
	
	pInfo.inputAssembly.topology = topology;

	pInfo.colorBlendAttachment.srcAlphaBlendFactor = 
		useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE;

	pInfo.depthStencil.depthTestEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE);
	pInfo.depthStencil.depthWriteEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE);

	pInfo.tessellationState.patchControlPoints = numPatchControlPoints;

	// Enable MSAA
	if (msaaSamples != VK_SAMPLE_COUNT_1_BIT)
	{
		pInfo.multisampling.rasterizationSamples = msaaSamples;
		pInfo.multisampling.sampleShadingEnable = VK_TRUE;
		pInfo.multisampling.minSampleShading = 0.25f;
	}

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pInfo.dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	pInfo.dynamicState.pDynamicStates = dynamicStates.data();

	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &pInfo.vertexInputInfo,
		.pInputAssemblyState = &pInfo.inputAssembly,
		.pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &pInfo.tessellationState : nullptr,
		.pViewportState = &pInfo.viewportState,
		.pRasterizationState = &pInfo.rasterizer,
		.pMultisampleState = &pInfo.multisampling,
		.pDepthStencilState = useDepth ? &pInfo.depthStencil : nullptr,
		.pColorBlendState = &pInfo.colorBlending,
		.pDynamicState = &pInfo.dynamicState,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VK_CHECK(vkCreateGraphicsPipelines(
		vkDev.GetDevice(),
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		pipeline));

	for (VulkanShader& s : shaderModules)
	{
		s.Destroy(vkDev.GetDevice());
	}
}

void RendererBase::UpdateUniformBuffer(
	VkDevice device,
	VulkanBuffer& buffer,
	const void* data,
	const size_t dataSize)
{
	VkDeviceMemory bufferMemory = buffer.bufferMemory_;

	void* mappedData = nullptr;
	vkMapMemory(
		device, 
		bufferMemory,
		0, 
		dataSize, 
		0, 
		&mappedData);
	memcpy(mappedData, data, dataSize);

	vkUnmapMemory(device, bufferMemory);
}

VkDescriptorSetLayoutBinding RendererBase::DescriptorSetLayoutBinding(
	uint32_t binding,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	uint32_t descriptorCount)
{
	return VkDescriptorSetLayoutBinding
	{
		.binding = binding,
		.descriptorType = descriptorType,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}

VkWriteDescriptorSet RendererBase::BufferWriteDescriptorSet(
	VkDescriptorSet ds,
	const VkDescriptorBufferInfo* bi,
	uint32_t bindIdx,
	VkDescriptorType dType)
{
	return 
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = ds,
		.dstBinding = bindIdx,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = dType,
		.pImageInfo = nullptr,
		.pBufferInfo = bi,
		.pTexelBufferView = nullptr
	};
}

VkWriteDescriptorSet RendererBase::ImageWriteDescriptorSet(
	VkDescriptorSet ds,
	const VkDescriptorImageInfo* ii,
	uint32_t bindIdx)
{
	return 
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = ds,
		.dstBinding = bindIdx,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = ii,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr
	};
}