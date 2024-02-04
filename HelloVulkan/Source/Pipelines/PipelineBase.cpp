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
	PipelineConfig config) :
	device_(vkDev.GetDevice()),
	config_(config)
{
}

// Destructor
PipelineBase::~PipelineBase()
{
	for (auto uboBuffer : cameraUBOBuffers_)
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
	const size_t swapChainImageSize = vkDev.GetSwapchainImageCount(); // TODO Set this as a parameter
	buffers.resize(swapChainImageSize);
	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		buffers[i].CreateBuffer(
			vkDev,
			uniformDataSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
	}
}

void PipelineBase::BindPipeline(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

	const VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(vkDev.GetFrameBufferWidth()),
		.height = static_cast<float>(vkDev.GetFrameBufferHeight()),
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
	// If this is compute pipeline, no need to recreate framebuffer
	if (config_.type_ == PipelineType::Compute)
	{
		return;
	}

	framebuffer_.Destroy();
	framebuffer_.Recreate(vkDev);
}

void PipelineBase::CreatePipelineLayout(
	VulkanDevice& vkDev,
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

	if (!pushConstantRanges.empty())
	{
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	}

	VK_CHECK(vkCreatePipelineLayout(vkDev.GetDevice(), &pipelineLayoutInfo, nullptr, pipelineLayout));
}

void PipelineBase::CreateGraphicsPipeline(
	VulkanDevice& vkDev,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<std::string>& shaderFiles,
	VkPipeline* pipeline)
{
	std::vector<VulkanShader> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize(shaderFiles.size());
	shaderModules.resize(shaderFiles.size());

	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i].c_str();
		VK_CHECK(shaderModules[i].Create(vkDev.GetDevice(), file));
		const VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Pipeline create info
	PipelineCreateInfo pInfo(vkDev);

	const std::vector<VkVertexInputBindingDescription> bindingDescriptions = 
		VertexData::GetBindingDescriptions();
	const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = 
		VertexData::GetAttributeDescriptions();

	if (config_.vertexBufferBind_)
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
	
	pInfo.inputAssembly.topology = config_.topology_;

	pInfo.colorBlendAttachment.srcAlphaBlendFactor = 
		config_.useBlending_ ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE;

	pInfo.depthStencil.depthTestEnable = static_cast<VkBool32>(config_.depthTest_ ? VK_TRUE : VK_FALSE);
	pInfo.depthStencil.depthWriteEnable = static_cast<VkBool32>(config_.depthWrite_ ? VK_TRUE : VK_FALSE);

	pInfo.tessellationState.patchControlPoints = config_.PatchControlPointsCount_;

	// Enable MSAA
	if (config_.msaaSamples_ != VK_SAMPLE_COUNT_1_BIT)
	{
		pInfo.multisampling.rasterizationSamples = config_.msaaSamples_;
		pInfo.multisampling.sampleShadingEnable = VK_TRUE;
		pInfo.multisampling.minSampleShading = 0.25f;
	}

	constexpr std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pInfo.dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	pInfo.dynamicState.pDynamicStates = dynamicStates.data();

	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &pInfo.vertexInputInfo,
		.pInputAssemblyState = &pInfo.inputAssembly,
		.pTessellationState = (config_.topology_ == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &pInfo.tessellationState : nullptr,
		.pViewportState = &pInfo.viewportState,
		.pRasterizationState = &pInfo.rasterizer,
		.pMultisampleState = &pInfo.multisampling,
		.pDepthStencilState = config_.depthTest_ ? &pInfo.depthStencil : nullptr,
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
	VulkanDevice& vkDev,
	const std::string& shaderFile)
{
	VulkanShader shader;
	shader.Create(vkDev.GetDevice(), shaderFile.c_str());

	const VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = {  // ShaderStageInfo
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = shader.GetShaderModule(),
			.pName = "main",
			.pSpecializationInfo = nullptr
		},
		.layout = pipelineLayout_,
		.basePipelineHandle = 0,
		.basePipelineIndex = 0
	};

	VK_CHECK(vkCreateComputePipelines(vkDev.GetDevice(), 0, 1, &computePipelineCreateInfo, nullptr, &pipeline_));

	shader.Destroy(vkDev.GetDevice());
}