#include "PipelineCubeFilter.h"
#include "PipelineCreateInfo.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "Configs.h"

PipelineCubeFilter::PipelineCubeFilter(
	VulkanDevice& vkDev, VulkanImage* inputCubemap) :
	PipelineBase(vkDev, 
		{
			.type_ = PipelineType::GraphicsOffScreen
		}
	) 
{
	// Create cube render pass
	renderPass_.CreateOffScreenCubemapRenderPass(vkDev, IBLConfig::CubeFormat);

	// Input cubemap
	uint32_t inputNumMipmap = NumMipMap(IBLConfig::InputCubeSideLength, IBLConfig::InputCubeSideLength);
	inputCubemap->GenerateMipmap(
		vkDev,
		inputNumMipmap,
		IBLConfig::InputCubeSideLength,
		IBLConfig::InputCubeSideLength,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
	inputCubemap->CreateSampler(
		vkDev,
		inputCubemapSampler_,
		0.f,
		static_cast<float>(inputNumMipmap)
	);

	CreateDescriptor(vkDev, inputCubemap);

	// Push constants
	std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0u,
		.size = sizeof(PushConstantCubeFilter)
	}};

	// Pipeline layout
	CreatePipelineLayout(vkDev, descriptor_.layout_, &pipelineLayout_, ranges);

	// Diffuse pipeline
	graphicsPipelines_.emplace_back(VK_NULL_HANDLE);
	CreateOffsreenGraphicsPipeline(
		vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "FullscreenTriangle.vert",
			AppConfig::ShaderFolder + "CubeFilterDiffuse.frag"
		},
		IBLConfig::OutputDiffuseSideLength,
		IBLConfig::OutputDiffuseSideLength,
		&graphicsPipelines_[0]
	);

	// Specular pipeline
	graphicsPipelines_.emplace_back(VK_NULL_HANDLE);
	CreateOffsreenGraphicsPipeline(
		vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "FullscreenTriangle.vert",
			AppConfig::ShaderFolder + "CubeFilterSpecular.frag"
		},
		IBLConfig::OutputSpecularSideLength,
		IBLConfig::OutputSpecularSideLength,
		&graphicsPipelines_[1]
	);
}

PipelineCubeFilter::~PipelineCubeFilter()
{
	vkDestroySampler(device_, inputCubemapSampler_, nullptr);

	for (VkPipeline& pipeline : graphicsPipelines_)
	{
		vkDestroyPipeline(device_, pipeline, nullptr);
	}
}

void PipelineCubeFilter::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void PipelineCubeFilter::InitializeOutputCubemap(
	VulkanDevice& vkDev, 
	VulkanImage* outputDiffuseCubemap,
	uint32_t numMipmap,
	uint32_t inputCubeSideLength)
{
	outputDiffuseCubemap->CreateImage(
		vkDev,
		inputCubeSideLength,
		inputCubeSideLength,
		numMipmap,
		IBLConfig::LayerCount,
		IBLConfig::CubeFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	outputDiffuseCubemap->CreateImageView(
		vkDev,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		IBLConfig::LayerCount,
		numMipmap);
}

void PipelineCubeFilter::CreateDescriptor(VulkanDevice& vkDev, VulkanImage* inputCubemap)
{
	// Pool
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.swapchainCount_ = 1u,
			.setCountPerSwapchain_ = 1u
		});

	// Layout
	descriptor_.CreateLayout(vkDev,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	VkDescriptorImageInfo imageInfo =
	{
		inputCubemapSampler_, // Local sampler created in the constructor
		inputCubemap->imageView_,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	descriptor_.CreateSet(
		vkDev,
		{
			{.imageInfoPtr_ = &imageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }
		},
		&descriptorSet_);
}

void PipelineCubeFilter::CreateOutputCubemapViews(VulkanDevice& vkDev,
	VulkanImage* outputCubemap,
	std::vector<std::vector<VkImageView>>& outputCubemapViews,
	uint32_t numMip)
{
	outputCubemapViews = 
		std::vector<std::vector<VkImageView>>(numMip, std::vector<VkImageView>(IBLConfig::LayerCount, VK_NULL_HANDLE));
	for (uint32_t a = 0; a < numMip; ++a)
	{
		outputCubemapViews[a] = {};
		for (uint32_t b = 0; b < IBLConfig::LayerCount; ++b)
		{
			VkImageSubresourceRange subresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };
			subresourceRange.baseMipLevel = a;
			subresourceRange.baseArrayLayer = b;

			const VkImageViewCreateInfo viewInfo =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = outputCubemap->image_,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = outputCubemap->imageFormat_,
				.components =
				{
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY
				},
				.subresourceRange = subresourceRange
			};
			outputCubemapViews[a].emplace_back();
			VK_CHECK(vkCreateImageView(vkDev.GetDevice(), &viewInfo, nullptr, &outputCubemapViews[a][b]));
		}
	}
}

void PipelineCubeFilter::CreateOffsreenGraphicsPipeline(
	VulkanDevice& vkDev,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<std::string>& shaderFiles,
	uint32_t viewportWidth,
	uint32_t viewportHeight,
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

	pInfo.viewport.width = viewportWidth;
	pInfo.viewport.height = viewportHeight;

	pInfo.scissor.extent = { viewportWidth, viewportHeight };

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | 
		VK_COLOR_COMPONENT_G_BIT | 
		VK_COLOR_COMPONENT_B_BIT | 
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(IBLConfig::LayerCount, colorBlendAttachment);

	pInfo.colorBlending.attachmentCount = IBLConfig::LayerCount;
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

// TODO Use VulkanFramebuffer
VkFramebuffer PipelineCubeFilter::CreateFrameBuffer(
	VulkanDevice& vkDev,
	std::vector<VkImageView> outputViews,
	uint32_t width,
	uint32_t height)
{
	VkFramebufferCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.renderPass = renderPass_.GetHandle(),
		.attachmentCount = static_cast<uint32_t>(outputViews.size()),
		.pAttachments = outputViews.data(),
		.width = width,
		.height = height,
		.layers = 1u
	};
	VkFramebuffer frameBuffer;
	VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &info, nullptr, &frameBuffer));
	return frameBuffer;
}

void PipelineCubeFilter::OffscreenRender(VulkanDevice& vkDev,
	VulkanImage* outputCubemap,
	CubeFilterType filterType)
{
	uint32_t outputMipMapCount = filterType == CubeFilterType::Diffuse ?
		1u :
		NumMipMap(IBLConfig::OutputSpecularSideLength, IBLConfig::OutputSpecularSideLength);

	uint32_t outputSideLength = filterType == CubeFilterType::Diffuse ?
		IBLConfig::OutputDiffuseSideLength :
		IBLConfig::OutputSpecularSideLength;

	InitializeOutputCubemap(vkDev, outputCubemap, outputMipMapCount, outputSideLength);

	// Create views from the output cubemap
	std::vector<std::vector<VkImageView>> outputViews;
	CreateOutputCubemapViews(vkDev, outputCubemap, outputViews, outputMipMapCount);

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

	// Select pipeline
	VkPipeline pipeline = graphicsPipelines_[static_cast<unsigned int>(filterType)];

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	std::vector<VkFramebuffer> usedFrameBuffers;

	for (int i = static_cast<int>(outputMipMapCount - 1u); i >= 0; --i)
	{
		uint32_t targetSize = outputSideLength >> i;

		VkFramebuffer frameBuffer = CreateFrameBuffer(vkDev, outputViews[i], targetSize, targetSize);
		usedFrameBuffers.push_back(frameBuffer);

		VkImageSubresourceRange  subresourceRange =
		{ VK_IMAGE_ASPECT_COLOR_BIT, static_cast<uint32_t>(i), 1u, 0u, 6u };

		outputCubemap->CreateBarrier({
			.commandBuffer = commandBuffer,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.sourceAccess = VK_ACCESS_SHADER_READ_BIT,
			.destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.destinationAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
			subresourceRange);

		PushConstantCubeFilter values{};
		values.roughness = filterType == CubeFilterType::Diffuse || outputMipMapCount == 1 ?
			0.f :
			static_cast<float>(i) / static_cast<float>(outputMipMapCount - 1);
		values.outputDiffuseSampleCount = IBLConfig::OutputDiffuseSampleCount;

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout_,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PushConstantCubeFilter), &values);

		const std::vector<VkClearValue> clearValues(6u, { 0.0f, 0.0f, 1.0f, 1.0f });

		renderPass_.BeginCubemapRenderPass(commandBuffer, frameBuffer, targetSize);

		vkCmdDraw(commandBuffer, 3, 1u, 0, 0);

		vkCmdEndRenderPass(commandBuffer);
	}

	// Convention is to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	outputCubemap->CreateBarrier({ 
		.commandBuffer = commandBuffer,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.sourceAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.destinationAccess = VK_ACCESS_SHADER_READ_BIT });

	vkDev.EndSingleTimeCommands(commandBuffer);

	// Destroy frame buffers
	for (VkFramebuffer& f : usedFrameBuffers)
	{
		vkDestroyFramebuffer(vkDev.GetDevice(), f, nullptr);
	}

	// Destroy image views
	for (auto& views : outputViews)
	{
		for (VkImageView& view : views)
		{
			vkDestroyImageView(vkDev.GetDevice(), view, nullptr);
		}
	}

	// Create a sampler for the output cubemap
	outputCubemap->CreateDefaultSampler(
		vkDev,
		0.0f,
		static_cast<float>(outputMipMapCount));
}