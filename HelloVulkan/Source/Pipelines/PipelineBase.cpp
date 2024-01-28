#include "PipelineBase.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "Mesh.h"
#include "PipelineCreateInfo.h"

#include <array>

// Constructor
PipelineBase::PipelineBase(
	const VulkanDevice& vkDev,
	PipelineFlags flags) :
	device_(vkDev.GetDevice()),
	flags_(flags)
{
}

// Destructor
PipelineBase::~PipelineBase()
{
	for (auto uboBuffer : perFrameUBOs_)
	{
		uboBuffer.Destroy(device_);
	}

	framebuffer_.Destroy();
	descriptor_.Destroy(device_);
	renderPass_.Destroy(device_);

	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	vkDestroyPipeline(device_, pipeline_, nullptr);
}

void PipelineBase::CreateUniformBuffers(
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

void PipelineBase::BindPipeline(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

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

void PipelineBase::OnWindowResized(VulkanDevice& vkDev)
{
	framebuffer_.Destroy();
	framebuffer_.Recreate(vkDev);
	
}

void PipelineBase::CreateDescriptorPool(
	VulkanDevice& vkDev,
	uint32_t uniformBufferCount,
	uint32_t storageBufferCount,
	uint32_t samplerCount,
	uint32_t setCountPerSwapchain,
	VkDescriptorPool* descriptorPool,
	VkDescriptorPoolCreateFlags flags)
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
		.flags = flags,
		.maxSets = static_cast<uint32_t>(imageCount * setCountPerSwapchain),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data()
	};

	VK_CHECK(vkCreateDescriptorPool(vkDev.GetDevice(), &poolInfo, nullptr, descriptorPool));
}

void PipelineBase::CreatePipelineLayout(
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

void PipelineBase::CreateGraphicsPipeline(
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

void PipelineBase::CreateComputePipeline(
	VkDevice device,
	VkShaderModule computeShader)
{
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = {  // ShaderStageInfo
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = computeShader,
			.pName = "main",
			.pSpecializationInfo = nullptr
		},
		.layout = pipelineLayout_,
		.basePipelineHandle = 0,
		.basePipelineIndex = 0
	};

	VK_CHECK(vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, nullptr, &pipeline_));
}

void PipelineBase::UpdateUniformBuffer(
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