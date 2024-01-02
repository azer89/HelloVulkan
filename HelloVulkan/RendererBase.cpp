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
	framebufferWidth_(vkDev.GetFrameBufferWidth()), 
	framebufferHeight_(vkDev.GetFrameBufferHeight()), 
	depthImage_(depthImage),
	offscreenColorImage_(offscreenColorImage)
{
}

// Destructor
RendererBase::~RendererBase()
{
	for (auto buf : perFrameUBOs_)
	{
		buf.Destroy(device_);
	}

	vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
	vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

	for (auto framebuffer : swapchainFramebuffers_)
	{
		vkDestroyFramebuffer(device_, framebuffer, nullptr);
	}

	renderPass_.Destroy(device_);

	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
}

void RendererBase::CreateUniformBuffers(
	VulkanDevice& vkDev,
	std::vector<VulkanBuffer>& buffers,
	size_t uniformDataSize)
{
	auto swapChainImageSize = vkDev.GetSwapChainImageSize();
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

void RendererBase::CreateOffScreenFramebuffer(
	VulkanDevice& vkDev,
	VulkanRenderPass renderPass,
	VkImageView colorImageView,
	VkImageView depthImageView,
	VkFramebuffer& framebuffer)
{
	std::array<VkImageView, 2> attachments = { colorImageView, depthImageView };

	const VkFramebufferCreateInfo framebufferInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass.GetHandle(),
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.width = vkDev.GetFrameBufferWidth(),
		.height = vkDev.GetFrameBufferHeight(),
		.layers = 1
	};

	VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &framebufferInfo, nullptr, &framebuffer));
}

void RendererBase::CreateOnScreenFramebuffers(
	VulkanDevice& vkDev,
	VulkanRenderPass renderPass)
{
	size_t swapchainImageSize = vkDev.GetSwapChainImageSize();

	swapchainFramebuffers_.resize(swapchainImageSize);

	for (size_t i = 0; i < swapchainImageSize; i++)
	{
		VkImageView swapchainImageView = vkDev.GetSwapchainImageView(i);

		const VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = renderPass.GetHandle(),
			.attachmentCount = 1u,
			.pAttachments = &swapchainImageView,
			.width = vkDev.GetFrameBufferWidth(),
			.height = vkDev.GetFrameBufferHeight(),
			.layers = 1
		};

		VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers_[i]));
	}
}

void RendererBase::CreateOnScreenFramebuffers(
	VulkanDevice& vkDev,
	VulkanRenderPass renderPass,
	VkImageView depthImageView)
{
	size_t swapchainImageSize = vkDev.GetSwapChainImageSize();

	swapchainFramebuffers_.resize(swapchainImageSize);

	for (size_t i = 0; i < swapchainImageSize; i++)
	{
		std::array<VkImageView, 2> attachments = {
			vkDev.GetSwapchainImageView(i),
			depthImageView
		};

		const VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = renderPass.GetHandle(),
			.attachmentCount = static_cast<uint32_t>((depthImageView == VK_NULL_HANDLE) ? 1 : 2),
			.pAttachments = attachments.data(),
			.width = vkDev.GetFrameBufferWidth(),
			.height = vkDev.GetFrameBufferHeight(),
			.layers = 1
		};

		VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers_[i]));
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
	const uint32_t imageCount = static_cast<uint32_t>(vkDev.GetSwapChainImageSize());

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
	VkPrimitiveTopology topology,
	bool useDepth,
	bool useBlending,
	bool dynamicScissorState,
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
		.pDynamicState = dynamicScissorState ? &pInfo.dynamicState : nullptr,
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