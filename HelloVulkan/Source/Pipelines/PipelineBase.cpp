#include "PipelineBase.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "Mesh.h"
#include "PipelineCreateInfo.h"

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
	for (auto uboBuffer : cameraUBOBuffers_)
	{
		uboBuffer.Destroy();
	}

	framebuffer_.Destroy();
	descriptor_.Destroy();
	renderPass_.Destroy();

	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	vkDestroyPipeline(device_, pipeline_, nullptr);
}

void PipelineBase::CreateMultipleUniformBuffers(
	VulkanContext& ctx,
	std::vector<VulkanBuffer>& buffers,
	uint32_t dataSize,
	size_t bufferCount)
{
	buffers.resize(bufferCount);
	for (size_t i = 0; i < bufferCount; i++)
	{
		buffers[i].CreateBuffer(
			ctx,
			dataSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);
	}
}

void PipelineBase::CreateIndirectBuffers(
	VulkanContext& ctx,
	Scene* scene,
	std::vector<VulkanBuffer>& indirectBuffers)
{
	const uint32_t meshSize = scene->GetMeshCount();
	const uint32_t indirectDataSize = meshSize * sizeof(VkDrawIndirectCommand);
	constexpr size_t numFrames = AppConfig::FrameOverlapCount;

	const std::vector<uint32_t> meshVertexCountArray = scene->GetMeshVertexCountArray();

	indirectBuffers.resize(numFrames);
	for (size_t i = 0; i < numFrames; ++i)
	{
		// Create
		indirectBuffers[i].CreateIndirectBuffer(ctx, indirectDataSize);
		// Map
		VkDrawIndirectCommand* data = indirectBuffers[i].MapIndirectBuffer();

		for (uint32_t j = 0; j < meshSize; ++j)
		{
			data[j] =
			{
				.vertexCount = static_cast<uint32_t>(meshVertexCountArray[j]),
				.instanceCount = 1u,
				.firstVertex = 0,
				.firstInstance = j
			};
		}

		// Unmap
		indirectBuffers[i].UnmapIndirectBuffer();
	}
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

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent =
	{
		static_cast<uint32_t>(w),
		static_cast<uint32_t>(h)
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

void PipelineBase::CreatePipelineLayout(
	VulkanContext& ctx,
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

	VK_CHECK(vkCreatePipelineLayout(ctx.GetDevice(), &pipelineLayoutInfo, nullptr, pipelineLayout));
}

void PipelineBase::CreateGraphicsPipeline(
	VulkanContext& ctx,
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
		VK_CHECK(shaderModules[i].Create(ctx.GetDevice(), file));
		const VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Pipeline create info
	PipelineCreateInfo pInfo(ctx);

	const std::vector<VkVertexInputBindingDescription> bindingDescriptions = Mesh::GetBindingDescriptions();
	const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Mesh::GetAttributeDescriptions();

	if (config_.vertexBufferBind_)
	{
		pInfo.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		pInfo.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		pInfo.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		pInfo.vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
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