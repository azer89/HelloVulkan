#include "PipelineBase.h"
#include "VertexData.h"
#include "Scene.h"

#include "VulkanCheck.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "VulkanPipelineCreateInfo.h"

#include <array>

// Constructor
PipelineBase::PipelineBase(
	const VulkanContext& ctx,
	const PipelineConfig& config) :
	device_(ctx.GetDevice()),
	config_(config)
{
}

// Destructor
PipelineBase::~PipelineBase()
{
	for (auto& uboBuffer : cameraUBOBuffers_)
	{
		uboBuffer.Destroy();
	}

	framebuffer_.Destroy();
	descriptorManager_.Destroy();
	renderPass_.Destroy();

	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	vkDestroyPipeline(device_, pipeline_, nullptr);
}

void PipelineBase::BindPipeline(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

	float w = static_cast<float>(ctx.GetSwapchainWidth());
	float h = static_cast<float>(ctx.GetSwapchainHeight());
	if (config_.customViewportSize_)
	{
		w = config_.viewportWidth_;
		h = config_.viewportHeight_;
	}
	const VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = w,
		.height = h,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor =
	{
		.offset = { 0, 0 },
		.extent =
		{
			static_cast<uint32_t>(w),
			static_cast<uint32_t>(h)
		}
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void PipelineBase::OnWindowResized(VulkanContext& ctx)
{
	// If this is compute pipeline, no need to recreate framebuffer
	if (config_.type_ == PipelineType::Compute)
	{
		return;
	}

	framebuffer_.Destroy();
	framebuffer_.Recreate(ctx);
}

void PipelineBase::CreatePipelineLayout(VulkanContext& ctx,
	VkDescriptorSetLayout dsLayout,
	VkPipelineLayout* pipelineLayout,
	uint32_t pushConstantSize,
	VkShaderStageFlags pushConstantShaderStage)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1, // Currently only one set per pipeline
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	VkPushConstantRange pcRange;
	if (pushConstantSize > 0)
	{
		pcRange =
		{
			.stageFlags = pushConstantShaderStage,
			.offset = 0u,
			.size = pushConstantSize,
		};
		pipelineLayoutInfo.pPushConstantRanges = &pcRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1u;
	}

	VK_CHECK(vkCreatePipelineLayout(ctx.GetDevice(), &pipelineLayoutInfo, nullptr, pipelineLayout));
}

void PipelineBase::CreateGraphicsPipeline(
	VulkanContext& ctx,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<std::string>& shaderFiles,
	VkPipeline* pipeline)
{
	std::vector<VulkanShader> shaderModules(shaderFiles.size(), {});
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaderFiles.size(), {});

	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i].c_str();
		VK_CHECK(shaderModules[i].Create(ctx.GetDevice(), file));
		const VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Add specialization constants if any
	specializationConstants_.Inject(shaderStages);

	// Pipeline create info
	VulkanPipelineCreateInfo pInfo(ctx);

	VkVertexInputBindingDescription bindingDescription = VertexUtility::GetBindingDescription();
	const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = VertexUtility::GetAttributeDescriptions();
	if (config_.vertexBufferBind_)
	{
		pInfo.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		pInfo.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		pInfo.vertexInputInfo.vertexBindingDescriptionCount = 1u;
		pInfo.vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	}
	
	pInfo.inputAssembly.topology = config_.topology_;

	if (overridingColorBlendAttachments.empty())
	{
		pInfo.colorBlendAttachment.srcAlphaBlendFactor =
			config_.useBlending_ ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
	}
	else
	{
		pInfo.colorBlending.attachmentCount = static_cast<uint32_t>(overridingColorBlendAttachments.size());
		pInfo.colorBlending.pAttachments = overridingColorBlendAttachments.data();
	}

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

	// Dynamic viewport and scissor are always active
	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	if (config_.lineWidth_ > 1.0f)
	{
		dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
	}
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
		ctx.GetDevice(),
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		pipeline));

	for (VulkanShader& s : shaderModules)
	{
		s.Destroy();
	}
}

void PipelineBase::CreateComputePipeline(
	VulkanContext& ctx,
	const std::string& shaderFile)
{
	VulkanShader shader;
	shader.Create(ctx.GetDevice(), shaderFile.c_str());

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

	VK_CHECK(vkCreateComputePipelines(ctx.GetDevice(), 0, 1, &computePipelineCreateInfo, nullptr, &pipeline_));

	shader.Destroy();
}

void PipelineBase::AddOverridingColorBlendAttachment(
	VkColorComponentFlags colorWriteMask,
	VkBool32 blendEnable)
{
	VkPipelineColorBlendAttachmentState attachment =
	{
		.blendEnable = blendEnable,
		.colorWriteMask = colorWriteMask,
	};
	overridingColorBlendAttachments.push_back(attachment);
}