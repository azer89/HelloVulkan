#include "RendererBase.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"

#include <array>

RendererBase::RendererBase(const VulkanDevice& vkDev, VulkanImage depthTexture) : 
	device_(vkDev.GetDevice()), 
	framebufferWidth_(vkDev.GetFrameBufferWidth()), 
	framebufferHeight_(vkDev.GetFrameBufferHeight()), 
	depthTexture_(depthTexture)
{
}

RendererBase::~RendererBase()
{
	for (auto buf : uniformBuffers_)
	{
		buf.Destroy(device_);
	}

	vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
	vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

	for (auto framebuffer : swapchainFramebuffers_)
	{
		vkDestroyFramebuffer(device_, framebuffer, nullptr);
	}

	vkDestroyRenderPass(device_, renderPass_, nullptr);
	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
}

void RendererBase::BeginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage)
{
	const VkRect2D screenRect = {
		.offset = { 0, 0 },
		.extent = {.width = framebufferWidth_, .height = framebufferHeight_ }
	};

	const VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = renderPass_,
		.framebuffer = swapchainFramebuffers_[currentImage],
		.renderArea = screenRect
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &descriptorSets_[currentImage], 0, nullptr);
}

bool RendererBase::CreateUniformBuffers(VulkanDevice& vkDev, size_t uniformDataSize)
{
	auto swapChainImageSize = vkDev.GetSwapChainImageSize();
	uniformBuffers_.resize(swapChainImageSize);
	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		bool res = uniformBuffers_[i].CreateBuffer(
			vkDev.GetDevice(),
			vkDev.GetPhysicalDevice(),
			uniformDataSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		if (!res)
		{
			std::cerr << "Cannot create uniform buffer\n";
			return false;
		}
	}
	return true;
}

bool RendererBase::CreateColorAndDepthRenderPass(
	VulkanDevice& vkDev,
	bool useDepth,
	VkRenderPass* renderPass,
	const RenderPassCreateInfo& ci,
	VkFormat colorFormat)
{
	const bool offscreenInt = ci.flags_ & eRenderPassBit_OffscreenInternal;
	const bool first = ci.flags_ & eRenderPassBit_First;
	const bool last = ci.flags_ & eRenderPassBit_Last;

	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = colorFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci.clearColor_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = first ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		.finalLayout = last ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = useDepth ? vkDev.FindDepthFormat() : VK_FORMAT_D32_SFLOAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci.clearDepth_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = ci.clearDepth_ ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	if (ci.flags_ & eRenderPassBit_Offscreen)
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::vector<VkSubpassDependency> dependencies = {
		/* VkSubpassDependency */ {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		}
	};

	if (ci.flags_ & eRenderPassBit_Offscreen)
	{
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Use subpass dependencies for layout transitions
		dependencies.resize(2);

		dependencies[0] = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};

		dependencies[1] = {
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};
	}

	const VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = useDepth ? &depthAttachmentRef : nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = static_cast<uint32_t>(useDepth ? 2 : 1),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data()
	};

	return (vkCreateRenderPass(vkDev.GetDevice(), &renderPassInfo, nullptr, renderPass) == VK_SUCCESS);
}

bool RendererBase::CreateColorAndDepthFramebuffers(
	VulkanDevice& vkDev,
	VkRenderPass renderPass,
	VkImageView depthImageView,
	std::vector<VkFramebuffer>& swapchainFramebuffers)
{
	size_t swapchainImageSize = vkDev.GetSwapChainImageSize();

	swapchainFramebuffers.resize(swapchainImageSize);

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
			.renderPass = renderPass,
			.attachmentCount = static_cast<uint32_t>((depthImageView == VK_NULL_HANDLE) ? 1 : 2),
			.pAttachments = attachments.data(),
			.width = vkDev.GetFrameBufferWidth(),
			.height = vkDev.GetFrameBufferHeight(),
			.layers = 1
		};

		VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers[i]));
	}

	return true;
}

bool RendererBase::CreateDescriptorPool(
	VulkanDevice& vkDev,
	uint32_t uniformBufferCount,
	uint32_t storageBufferCount,
	uint32_t samplerCount,
	VkDescriptorPool* descriptorPool)
{
	const uint32_t imageCount = static_cast<uint32_t>(vkDev.GetSwapChainImageSize());

	std::vector<VkDescriptorPoolSize> poolSizes;

	if (uniformBufferCount)
		poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = imageCount * uniformBufferCount });

	if (storageBufferCount)
		poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = imageCount * storageBufferCount });

	if (samplerCount)
		poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = imageCount * samplerCount });

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = static_cast<uint32_t>(imageCount),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data()
	};

	return (vkCreateDescriptorPool(vkDev.GetDevice(), &poolInfo, nullptr, descriptorPool) == VK_SUCCESS);
}

bool RendererBase::CreatePipelineLayout(
	VkDevice device, 
	VkDescriptorSetLayout dsLayout, 
	VkPipelineLayout* pipelineLayout)
{
	const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	return (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout) == VK_SUCCESS);
}

bool RendererBase::CreateGraphicsPipeline(
	VulkanDevice& vkDev,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<const char*>& shaderFiles,
	VkPipeline* pipeline,
	VkPrimitiveTopology topology,
	bool useDepth,
	bool useBlending,
	bool dynamicScissorState,
	int32_t customWidth,
	int32_t customHeight,
	uint32_t numPatchControlPoints)
{
	std::vector<VulkanShader> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize(shaderFiles.size());
	shaderModules.resize(shaderFiles.size());

	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i];
		VK_CHECK(shaderModules[i].Create(vkDev.GetDevice(), file));
		VkShaderStageFlagBits stage = GLSLangShaderStageToVulkan(GLSLangShaderStageFromFileName(file));
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		/* The only difference from createGraphicsPipeline() */
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};

	const VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(customWidth > 0 ? customWidth : vkDev.GetFrameBufferWidth()),
		.height = static_cast<float>(customHeight > 0 ? customHeight : vkDev.GetFrameBufferHeight()),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	const VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = { customWidth > 0 ? customWidth : vkDev.GetFrameBufferWidth(), customHeight > 0 ? customHeight : vkDev.GetFrameBufferHeight()}
	};

	const VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	const VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f
	};

	const VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	const VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	const VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
		.depthWriteEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	VkDynamicState dynamicStateElt = VK_DYNAMIC_STATE_SCISSOR;

	const VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = 1,
		.pDynamicStates = &dynamicStateElt
	};

	const VkPipelineTessellationStateCreateInfo tessellationState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.patchControlPoints = numPatchControlPoints
	};

	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &tessellationState : nullptr,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = useDepth ? &depthStencil : nullptr,
		.pColorBlendState = &colorBlending,
		.pDynamicState = dynamicScissorState ? &dynamicState : nullptr,
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

	for (auto s : shaderModules)
	{
		s.Destroy(vkDev.GetDevice());
	}

	return true;
}

void RendererBase::UploadBufferData(
	VulkanDevice& vkDev,
	const VkDeviceMemory& bufferMemory,
	VkDeviceSize deviceOffset,
	const void* data,
	const size_t dataSize)
{
	void* mappedData = nullptr;
	vkMapMemory(vkDev.GetDevice(), bufferMemory, deviceOffset, dataSize, 0, &mappedData);
	memcpy(mappedData, data, dataSize);
	vkUnmapMemory(vkDev.GetDevice(), bufferMemory);
}

void RendererBase::CopyBufferToImage(
	VulkanDevice& vkDev,
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height,
	uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = vkDev.BeginSingleTimeCommands();

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		},
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	vkDev.EndSingleTimeCommands(commandBuffer);
}

void RendererBase::CopyImageToBuffer(
	VulkanDevice& vkDev,
	VkImage image,
	VkBuffer buffer,
	uint32_t width,
	uint32_t height,
	uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = vkDev.BeginSingleTimeCommands();

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		},
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
	};

	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

	vkDev.EndSingleTimeCommands(commandBuffer);
}

size_t RendererBase::AllocateVertexBuffer(
	VulkanDevice& vkDev,
	VulkanBuffer* buffer,
	size_t vertexDataSize,
	const void* vertexData,
	size_t indexDataSize,
	const void* indexData)
{
	VkDeviceSize bufferSize = vertexDataSize + indexDataSize;

	VulkanBuffer stagingBuffer;
	stagingBuffer.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void* data;
	vkMapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_, 0, bufferSize, 0, &data);
	memcpy(data, vertexData, vertexDataSize);
	memcpy((unsigned char*)data + vertexDataSize, indexData, indexDataSize);
	vkUnmapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_);

	buffer->CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	buffer->CopyFrom(vkDev, stagingBuffer.buffer_, bufferSize);

	stagingBuffer.Destroy(vkDev.GetDevice());

	return bufferSize;
}