#include "RendererEquirect2Cube.h"
#include "PipelineCreateInfo.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "AppSettings.h"

namespace CubeSettings
{
	const uint32_t sideLength = 1024;
	const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
	const uint32_t layerCount = 6;
}

RendererEquirect2Cube::RendererEquirect2Cube(
	VulkanDevice& vkDev, 
	const std::string& hdrFile) :
	RendererBase(vkDev, nullptr)
{
	InitializeHDRImage(vkDev, hdrFile);
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
		cubeRenderPass_,
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
	vkDestroyRenderPass(device_, cubeRenderPass_, nullptr);
	inputHDRImage_.Destroy(device_);
	vkDestroyFramebuffer(device_, frameBuffer_, nullptr);
}

void RendererEquirect2Cube::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void RendererEquirect2Cube::InitializeCubemap(VulkanDevice& vkDev, VulkanImage* cubemap)
{
	uint32_t mipmapCount = NumMipMap(CubeSettings::sideLength, CubeSettings::sideLength);

	cubemap->CreateImage(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		CubeSettings::sideLength,
		CubeSettings::sideLength,
		mipmapCount,
		CubeSettings::layerCount,
		CubeSettings::format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	cubemap->CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		CubeSettings::layerCount,
		mipmapCount);
}

void RendererEquirect2Cube::InitializeHDRImage(VulkanDevice& vkDev, const std::string& hdrFile)
{
	inputHDRImage_.CreateFromHDR(vkDev, hdrFile.c_str());
	inputHDRImage_.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT);
	inputHDRImage_.CreateDefaultSampler(
		vkDev.GetDevice(),
		0.f,
		1.f);
}

void RendererEquirect2Cube::CreateRenderPass(VulkanDevice& vkDev)
{
	VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::vector<VkAttachmentDescription> m_attachments;
	std::vector<VkAttachmentReference> m_attachmentRefs;

	for (uint32_t face = 0; face < CubeSettings::layerCount; ++face)
	{
		VkAttachmentDescription info =
		{
			.flags = 0u,
			.format = CubeSettings::format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = finalLayout
		};

		VkAttachmentReference ref =
		{
			.attachment = static_cast<uint32_t>(face),
			.layout = finalLayout,
		};

		m_attachments.push_back(info);
		m_attachmentRefs.push_back(ref);
	}

	VkSubpassDescription subpassDesc =
	{
		.flags = 0u,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = static_cast<uint32_t>(m_attachmentRefs.size()),
		.pColorAttachments = m_attachmentRefs.data()
	};

	VkRenderPassCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.attachmentCount = static_cast<uint32_t>(m_attachments.size()),
		.pAttachments = m_attachments.data(),
		.subpassCount = 1u,
		.pSubpasses = &subpassDesc
	};

	VK_CHECK(vkCreateRenderPass(vkDev.GetDevice(), &createInfo, nullptr, &cubeRenderPass_));
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
		inputHDRImage_.defaultImageSampler_,
		inputHDRImage_.imageView_,
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

	pInfo.viewport.width = CubeSettings::sideLength;
	pInfo.viewport.height = CubeSettings::sideLength;

	pInfo.scissor.extent = { CubeSettings::sideLength, CubeSettings::sideLength };

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(CubeSettings::layerCount, colorBlendAttachment);

	pInfo.colorBlending.attachmentCount = CubeSettings::layerCount;
	pInfo.colorBlending.pAttachments = colorBlendAttachments.data();

	// No depth test
	pInfo.depthStencil.depthTestEnable = VK_FALSE;
	pInfo.depthStencil.depthWriteEnable = VK_FALSE;
	
	pInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
	VkFramebufferCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.renderPass = cubeRenderPass_,
		.attachmentCount = static_cast<uint32_t>(outputViews.size()),
		.pAttachments = outputViews.data(),
		.width = CubeSettings::sideLength,
		.height = CubeSettings::sideLength,
		.layers = 1u,
	};

	VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &info, nullptr, &frameBuffer_));
}

// TODO Move this to VulkanImage
void RendererEquirect2Cube::CreateCubemapViews(
	VulkanDevice& vkDev, 
	VulkanImage* cubemap,
	std::vector<VkImageView>& cubemapViews)
{
	cubemapViews = std::vector<VkImageView>(CubeSettings::layerCount, VK_NULL_HANDLE);
	for (size_t i = 0; i < CubeSettings::layerCount; i++)
	{
		const VkImageViewCreateInfo viewInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = cubemap->image_,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = cubemap->imageFormat_,
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

		VK_CHECK(vkCreateImageView(vkDev.GetDevice(), &viewInfo, nullptr, &cubemapViews[i]));
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

	outputEnvMap->CreateBarrier({
		.commandBuffer = commandBuffer,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.sourceAccess = VK_ACCESS_SHADER_READ_BIT,
		.destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.destinationAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	});

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
	info.renderPass = cubeRenderPass_;
	info.framebuffer = frameBuffer_;
	info.renderArea = { 0u, 0u, CubeSettings::sideLength, CubeSettings::sideLength };
	info.clearValueCount = static_cast<uint32_t>(clearValues.size());
	info.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdDraw(commandBuffer, 3, 1u, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	// Convention is to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	outputEnvMap->CreateBarrier({
		.commandBuffer = commandBuffer,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
		.sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.sourceAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.destinationAccess = VK_ACCESS_SHADER_READ_BIT
	});

	vkDev.EndSingleTimeCommands(commandBuffer);

	// Create a sampler for the output cubemap
	outputEnvMap->CreateDefaultSampler(vkDev.GetDevice());

	// Destroy image views
	for (size_t i = 0; i < CubeSettings::layerCount; i++)
	{
		vkDestroyImageView(vkDev.GetDevice(), outputViews[i], nullptr);
	}
}