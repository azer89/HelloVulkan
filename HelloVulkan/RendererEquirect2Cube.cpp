#include "RendererEquirect2Cube.h"
#include "PipelineCreateInfo.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "AppSettings.h"

const uint32_t cubemapSideLength = 1024;
const VkFormat cubeMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
const uint32_t layerCount = 6;

RendererEquirect2Cube::RendererEquirect2Cube(
	VulkanDevice& vkDev, 
	const std::string& hdrFile) :
	RendererBase(vkDev, nullptr)
{
	InitializeHDRTexture(vkDev, hdrFile);
	CreateRenderPass(vkDev);

	CreateDescriptorPool(
		vkDev,
		0, // UBO
		0, // SSBO
		1, // Sampler
		1, // Descriptor count per swapchain
		&descriptorPool_);

	CreateDescriptorLayout(vkDev);
	CreateDescriptorSet(vkDev);
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);

	CreateOffscreenGraphicsPipeline(
		vkDev,
		renderPass_,
		pipelineLayout_,
		{
			AppSettings::ShaderFolder + "FullscreenTriangle.vert",
			AppSettings::ShaderFolder + "Equirect2Cube.frag"
		},
		&graphicsPipeline_
	);
}

RendererEquirect2Cube::~RendererEquirect2Cube()
{
	inputHDRTexture_.Destroy(device_);
	vkDestroyFramebuffer(device_, frameBuffer_, nullptr);
}

void RendererEquirect2Cube::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void RendererEquirect2Cube::InitializeCubemap(VulkanDevice& vkDev, VulkanImage* outputCubemap)
{
	uint32_t mipmapCount = NumMipMap(cubemapSideLength, cubemapSideLength);

	outputCubemap->CreateImage(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		cubemapSideLength,
		cubemapSideLength,
		mipmapCount,
		layerCount,
		cubeMapFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	outputCubemap->CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		layerCount,
		mipmapCount);
}

void RendererEquirect2Cube::InitializeHDRTexture(VulkanDevice& vkDev, const std::string& hdrFile)
{
	inputHDRTexture_.CreateHDRImage(vkDev, hdrFile.c_str());
	inputHDRTexture_.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT);
	inputHDRTexture_.CreateTextureSampler(
		vkDev.GetDevice(),
		0.f,
		1.f);
}

void RendererEquirect2Cube::CreateRenderPass(VulkanDevice& vkDev)
{
	VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

void RendererEquirect2Cube::CreateDescriptorLayout(VulkanDevice& vkDev)
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

void RendererEquirect2Cube::CreateDescriptorSet(VulkanDevice& vkDev)
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

	VkDescriptorImageInfo imageInfo =
	{
		inputHDRTexture_.sampler_,
		inputHDRTexture_.imageView_,
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

void RendererEquirect2Cube::CreateOffscreenGraphicsPipeline(
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
		VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Pipeline create info
	PipelineCreateInfo pInfo(vkDev);

	pInfo.viewport.width = cubemapSideLength;
	pInfo.viewport.height = cubemapSideLength;

	pInfo.scissor.extent = { cubemapSideLength, cubemapSideLength };

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

	/*VkDynamicState dynamicStates[] =
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
	*/
	pInfo.dynamicState.dynamicStateCount = 0;
	pInfo.dynamicState.pDynamicStates = nullptr;

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

	// disable multisampling
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
}

void RendererEquirect2Cube::CreateFrameBuffer(
	VulkanDevice& vkDev, 
	std::vector<VkImageView> outputViews)
{
	VkFramebufferCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.renderPass = renderPass_;
	info.attachmentCount = static_cast<uint32_t>(outputViews.size());
	info.pAttachments = outputViews.data();
	info.width = cubemapSideLength;
	info.height = cubemapSideLength;
	info.layers = 1u;
	info.flags = 0u;

	VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &info, nullptr, &frameBuffer_));
}

// TODO Move this to VulkanImage
void RendererEquirect2Cube::CreateCubemapViews(
	VulkanDevice& vkDev, 
	VulkanImage* cubemapTexture,
	std::vector<VkImageView>& cubeMapViews)
{
	cubeMapViews = std::vector<VkImageView>(layerCount, VK_NULL_HANDLE);
	for (size_t i = 0; i < layerCount; i++)
	{
		const VkImageViewCreateInfo viewInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = cubemapTexture->image_,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = cubemapTexture->imageFormat_,
			.components =
			{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange =
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				1u,
				static_cast<uint32_t>(i),
				1u
			}
		};

		VK_CHECK(vkCreateImageView(vkDev.GetDevice(), &viewInfo, nullptr, &cubeMapViews[i]));
	}
}

void RendererEquirect2Cube::OffscreenRender(VulkanDevice& vkDev, VulkanImage* outputEnvMap)
{
	// Initialize output cubemap
	InitializeCubemap(vkDev, outputEnvMap);

	// Create views from the output cubemap
	std::vector<VkImageView> outputViews;
	CreateCubemapViews(vkDev, outputEnvMap, outputViews);

	CreateFrameBuffer(vkDev, outputViews);

	VkCommandBuffer commandBuffer = vkDev.BeginSingleTimeCommands();

	outputEnvMap->CreateBarrier(
		commandBuffer,
		VK_IMAGE_LAYOUT_UNDEFINED, // oldLayout
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // newLayout
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // srcStage
		VK_ACCESS_SHADER_READ_BIT, // srcAccess
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStage
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT // dstAccess
	);

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

	const std::vector<VkClearValue> clearValues(6u, { 0.0f, 0.0f, 1.0f, 1.0f });

	VkRenderPassBeginInfo info{};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.pNext = nullptr;
	info.renderPass = renderPass_;
	info.framebuffer = frameBuffer_;
	info.renderArea = { 0u, 0u, cubemapSideLength, cubemapSideLength };
	info.clearValueCount = static_cast<uint32_t>(clearValues.size());
	info.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdDraw(commandBuffer, 3, 1u, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	// Convention is to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	outputEnvMap->CreateBarrier(
		commandBuffer,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // oldLayout
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // newLayout
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStage
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccess
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // dstStage
		VK_ACCESS_SHADER_READ_BIT // dstAccess
	);

	vkDev.EndSingleTimeCommands(commandBuffer);

	// Create a sampler for the output cubemap
	outputEnvMap->CreateTextureSampler(vkDev.GetDevice());

	// Destroy image views
	for (size_t i = 0; i < layerCount; i++)
	{
		vkDestroyImageView(vkDev.GetDevice(), outputViews[i], nullptr);
	}
}