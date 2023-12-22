#include "RendererCubeFilter.h"
#include "PipelineCreateInfo.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "CubePushConstant.h"
#include "AppSettings.h"

const uint32_t outputMipmapCount = 1u; // TODO adjust for prefilter/specular cubemap
const uint32_t inputSize = 1024;
const uint32_t outputSize = 128;
const VkFormat cubeMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
const uint32_t layerCount = 6;

RendererCubeFilter::RendererCubeFilter(
	VulkanDevice& vkDev, VulkanTexture* cubemapTexture) :
	RendererBase(vkDev, nullptr)
{
	CreateRenderPass(vkDev);

	CreateDescriptorPool(
		vkDev,
		0, // UBO
		0, // SSBO
		1, // Sampler
		1, // Descriptor count per swapchain
		&descriptorPool_);

	cubemapTexture->CreateTextureSampler(
		vkDev.GetDevice(), 
		inputEnvMapSampler_,
		0.f,
		static_cast<float>(NumMipMap(inputSize, inputSize))
		);
	CreateDescriptorLayout(vkDev);
	CreateDescriptorSet(vkDev, cubemapTexture);

	// Push constants
	std::vector<VkPushConstantRange> ranges(1u);
	VkPushConstantRange& range = ranges.front();
	range.offset = 0u;
	range.size = sizeof(PushConstant);
	range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Pipeline layout
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_, ranges);

	// Graphics Pipeline
	std::string vertFile = AppSettings::ShaderFolder + "fullscreen_triangle.vert";
	std::string fragFile = AppSettings::ShaderFolder + "cube_filter.frag";
	CreateOffsreenGraphicsPipeline(
		vkDev,
		renderPass_,
		pipelineLayout_,
		{
			vertFile.c_str(),
			fragFile.c_str()
		},
		&graphicsPipeline_
	);
}

RendererCubeFilter::~RendererCubeFilter()
{
	vkDestroySampler(device_, inputEnvMapSampler_, nullptr);
	vkDestroyFramebuffer(device_, frameBuffer_, nullptr);
}

void RendererCubeFilter::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void RendererCubeFilter::InitializeIrradianceTexture(VulkanDevice& vkDev, VulkanTexture* irradianceTexture)
{
	irradianceTexture->image_.CreateImage(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		outputSize,
		outputSize,
		outputMipmapCount,
		layerCount,
		cubeMapFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	irradianceTexture->image_.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		layerCount,
		outputMipmapCount);
}

void RendererCubeFilter::CreateRenderPass(VulkanDevice& vkDev)
{
	const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::vector<VkAttachmentDescription> m_attachments;
	std::vector<VkAttachmentReference> m_attachmentRefs;

	for (int face = 0; face < layerCount; ++face)
	{
		VkAttachmentDescription info{};
		info.flags = 0u;
		info.format = cubeMapFormat;
		info.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.finalLayout = finalLayout;
		info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		VkAttachmentReference ref{};
		ref.attachment = static_cast<uint32_t>(face);
		ref.layout = finalLayout;

		m_attachments.push_back(info);
		m_attachmentRefs.push_back(ref);
	}

	VkSubpassDescription m_subpass{};
	m_subpass.flags = 0u;
	m_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	m_subpass.colorAttachmentCount = static_cast<uint32_t>(m_attachmentRefs.size());
	m_subpass.pColorAttachments = m_attachmentRefs.data();

	VkRenderPassCreateInfo m_info{};
	m_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_info.pNext = nullptr;
	m_info.flags = 0u;
	m_info.pSubpasses = &m_subpass;
	m_info.subpassCount = 1u;
	m_info.pAttachments = m_attachments.data();
	m_info.attachmentCount = static_cast<uint32_t>(m_attachments.size());

	VK_CHECK(vkCreateRenderPass(vkDev.GetDevice(), &m_info, nullptr, &renderPass_));
}

void RendererCubeFilter::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	uint32_t bindingIndex = 0;

	// Input HDR
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	const VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout_));
}

void RendererCubeFilter::CreateDescriptorSet(VulkanDevice& vkDev, VulkanTexture* cubemapTexture)
{
	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = 1u,
		.pSetLayouts = &descriptorSetLayout_
	};

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, &descriptorSet_));

	uint32_t bindIndex = 0;
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	const VkDescriptorImageInfo imageInfo =
	{
		inputEnvMapSampler_, // Local sampler created in the constructor
		cubemapTexture->image_.imageView_,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	descriptorWrites.emplace_back
	(
		ImageWriteDescriptorSet(
			descriptorSet_,
			&imageInfo,
			bindIndex++)
	);

	vkUpdateDescriptorSets
	(
		vkDev.GetDevice(),
		static_cast<uint32_t>(descriptorWrites.size()),
		descriptorWrites.data(),
		0,
		nullptr
	);
}

// TODO Move this to VulkanTexture
void RendererCubeFilter::CreateCubemapViews(VulkanDevice& vkDev,
	VulkanTexture* cubemapTexture,
	std::vector<VkImageView>& cubemapViews)
{
	cubemapViews = std::vector<VkImageView>(layerCount, VK_NULL_HANDLE);
	for (size_t i = 0; i < layerCount; i++)
	{
		VkImageSubresourceRange subresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };
		subresourceRange.baseMipLevel = 0;
		subresourceRange.baseArrayLayer = i;

		const VkImageViewCreateInfo viewInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = cubemapTexture->image_.image_,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = cubemapTexture->image_.imageFormat_,
			.components =
			{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = subresourceRange
		};

		VK_CHECK(vkCreateImageView(vkDev.GetDevice(), &viewInfo, nullptr, &cubemapViews[i]));
	}
}

bool RendererCubeFilter::CreateOffsreenGraphicsPipeline(
	VulkanDevice& vkDev,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<const char*>& shaderFiles,
	VkPipeline* pipeline)
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

	// Pipeline create info
	PipelineCreateInfo pInfo(vkDev);

	pInfo.viewport.width = outputSize;
	pInfo.viewport.height = outputSize;

	pInfo.scissor.extent = { outputSize, outputSize };

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(layerCount, colorBlendAttachment);

	pInfo.colorBlending.attachmentCount = layerCount;
	pInfo.colorBlending.pAttachments = colorBlendAttachments.data();

	// No depth test
	pInfo.depthStencil.depthTestEnable = VK_FALSE;
	pInfo.depthStencil.depthWriteEnable = VK_FALSE;

	VkDynamicState dynamicStates[] =
	{
		VK_DYNAMIC_STATE_LINE_WIDTH,
		VK_DYNAMIC_STATE_DEPTH_BIAS,
		VK_DYNAMIC_STATE_BLEND_CONSTANTS,
		VK_DYNAMIC_STATE_DEPTH_BOUNDS,
		VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
		VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
		VK_DYNAMIC_STATE_STENCIL_REFERENCE
	};

	pInfo.dynamicState.dynamicStateCount = static_cast<uint32_t>(sizeof(dynamicStates) / sizeof(VkDynamicState));
	pInfo.dynamicState.pDynamicStates = dynamicStates;

	pInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pInfo.rasterizer.pNext = nullptr;
	pInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
	pInfo.rasterizer.depthBiasClamp = 0.f;
	pInfo.rasterizer.depthBiasConstantFactor = 1.f;
	pInfo.rasterizer.depthBiasEnable = VK_FALSE;
	pInfo.rasterizer.depthBiasSlopeFactor = 1.f;
	pInfo.rasterizer.depthClampEnable = VK_FALSE;
	pInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pInfo.rasterizer.lineWidth = 1.f;
	pInfo.rasterizer.rasterizerDiscardEnable = VK_FALSE; 
	pInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	// Disable multisampling
	pInfo.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pInfo.multisampling.pNext = nullptr;
	pInfo.multisampling.alphaToCoverageEnable = VK_FALSE;
	pInfo.multisampling.alphaToOneEnable = VK_FALSE;
	pInfo.multisampling.flags = 0u;
	pInfo.multisampling.minSampleShading = 0.f;
	pInfo.multisampling.pSampleMask = nullptr;
	pInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pInfo.multisampling.sampleShadingEnable = VK_FALSE;

	pInfo.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pInfo.depthStencil.depthTestEnable = VK_FALSE;
	pInfo.depthStencil.depthWriteEnable = VK_FALSE;
	pInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	pInfo.depthStencil.depthBoundsTestEnable = VK_FALSE;
	pInfo.depthStencil.minDepthBounds = 0.0f; // Optional
	pInfo.depthStencil.maxDepthBounds = 1.0f; // Optional
	pInfo.depthStencil.stencilTestEnable = VK_FALSE;
	pInfo.depthStencil.front = {}; // Optional
	pInfo.depthStencil.back = {}; // Optional

	pInfo.tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	pInfo.tessellationState.pNext = nullptr;
	pInfo.tessellationState.flags = 0u;
	pInfo.tessellationState.patchControlPoints = 0u;

	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &pInfo.vertexInputInfo,
		.pInputAssemblyState = &pInfo.inputAssembly,
		.pTessellationState = &pInfo.tessellationState,
		.pViewportState = &pInfo.viewportState,
		.pRasterizationState = &pInfo.rasterizer,
		.pMultisampleState = &pInfo.multisampling,
		.pDepthStencilState = &pInfo.depthStencil,
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

	for (auto s : shaderModules)
	{
		s.Destroy(vkDev.GetDevice());
	}

	return true;
}

VkFramebuffer RendererCubeFilter::CreateFrameBuffer(
	VulkanDevice& vkDev,
	std::vector<VkImageView> outputViews)
{
	VkFramebufferCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.renderPass = renderPass_;
	info.attachmentCount = static_cast<uint32_t>(outputViews.size());
	info.pAttachments = outputViews.data();
	info.width = outputSize;
	info.height = outputSize;
	info.layers = 1u;
	info.flags = 0u;

	VkFramebuffer frameBuffer;
	VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &info, nullptr, &frameBuffer));
	return frameBuffer;
}

void RendererCubeFilter::OffscreenRender(VulkanDevice& vkDev, 
	VulkanTexture* cubemapTexture, 
	VulkanTexture* irradianceTexture)
{
	uint32_t inputMipMapCount = NumMipMap(inputSize, inputSize);

	InitializeIrradianceTexture(vkDev, irradianceTexture);

	// Create views from the output cubemap
	std::vector<VkImageView> outputViews;
	CreateCubemapViews(vkDev, irradianceTexture, outputViews);

	cubemapTexture->image_.GenerateMipmap(
		vkDev,
		inputMipMapCount,
		inputSize,
		inputSize,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);
	
	VkCommandBuffer commandBuffer = vkDev.BeginSingleTimeCommands();

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0,
		1,
		&descriptorSet_,
		0,
		nullptr);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
	
	std::vector<VkFramebuffer> usedFrameBuffers;

	for (int i = static_cast<int>(outputMipmapCount - 1u); i >= 0; --i)
	{
		VkFramebuffer frameBuffer = CreateFrameBuffer(vkDev, outputViews);
		usedFrameBuffers.push_back(frameBuffer);

		VkImageSubresourceRange  subresourceRange = 
		{ VK_IMAGE_ASPECT_COLOR_BIT, static_cast<uint32_t>(i), 1u, 0u, 6u };

		irradianceTexture->image_.CreateBarrier(commandBuffer,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,//src stage, access
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // dst stage, access
			subresourceRange);

		PushConstant values{};
		values.roughness = 1.f; 
		values.sampleCount = 1024;
		values.mipLevel = static_cast<uint32_t>(i);
		values.width = inputSize; // TODO Rename
		values.lodBias = 0;
		values.distribution = Distribution::Lambertian;

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout_, 
			VK_SHADER_STAGE_FRAGMENT_BIT, 
			0, 
			sizeof(PushConstant), &values);

		const std::vector<VkClearValue> clearValues(6u, { 0.0f, 0.0f, 1.0f, 1.0f });

		VkRenderPassBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.pNext = nullptr;
		info.renderPass = renderPass_;
		info.framebuffer = frameBuffer;
		info.renderArea = { 0u, 0u, outputSize, outputSize };
		info.clearValueCount = static_cast<uint32_t>(clearValues.size());
		info.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdDraw(commandBuffer, 3, 1u, 0, 0);

		vkCmdEndRenderPass(commandBuffer);
	}
	
	vkDev.EndSingleTimeCommands(commandBuffer);

	// Destroy frame buffers
	for (VkFramebuffer& f : usedFrameBuffers)
	{
		vkDestroyFramebuffer(vkDev.GetDevice(), f, nullptr);
	}

	// Destroy image views
	for (size_t i = 0; i < layerCount; i++)
	{
		vkDestroyImageView(vkDev.GetDevice(), outputViews[i], nullptr);
	}

	// Create a sampler for the output cubemap
	irradianceTexture->CreateTextureSampler(vkDev.GetDevice());

	// Transition to a new layout
	irradianceTexture->image_.TransitionImageLayout(
		vkDev,
		irradianceTexture->image_.imageFormat_,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		irradianceTexture->image_.layerCount_,
		irradianceTexture->image_.mipCount_
	);
}