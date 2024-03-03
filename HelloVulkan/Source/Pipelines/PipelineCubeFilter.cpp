#include "PipelineCubeFilter.h"
#include "PipelineCreateInfo.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanFramebuffer.h"
#include "Configs.h"

PipelineCubeFilter::PipelineCubeFilter(
	VulkanContext& ctx, VulkanImage* inputCubemap) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen
		}
	) 
{
	// Create cube render pass
	renderPass_.CreateOffScreenCubemapRenderPass(ctx, IBLConfig::CubeFormat);

	// Input cubemap
	const uint32_t inputNumMipmap = Utility::MipMapCount(IBLConfig::InputCubeSideLength);
	inputCubemap->GenerateMipmap(
		ctx,
		inputNumMipmap,
		IBLConfig::InputCubeSideLength,
		IBLConfig::InputCubeSideLength,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
	inputCubemap->CreateSampler(
		ctx,
		inputCubemapSampler_,
		0.f,
		static_cast<float>(inputNumMipmap)
	);

	CreateDescriptor(ctx, inputCubemap);

	// Push constants
	std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0u,
		.size = sizeof(PushConstCubeFilter)
	}};

	// Pipeline layout
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, ranges);

	// Diffuse pipeline
	graphicsPipelines_.emplace_back(VK_NULL_HANDLE);
	CreateOffscreenGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "FullscreenTriangle.vert",
			AppConfig::ShaderFolder + "CubeFilterDiffuse.frag"
		},
		IBLConfig::OutputDiffuseSideLength,
		IBLConfig::OutputDiffuseSideLength,
		&(graphicsPipelines_[0])
	);

	// Specular pipeline
	graphicsPipelines_.emplace_back(VK_NULL_HANDLE);
	CreateOffscreenGraphicsPipeline(
		ctx,
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

void PipelineCubeFilter::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
}

void PipelineCubeFilter::InitializeOutputCubemap(
	VulkanContext& ctx, 
	VulkanImage* outputDiffuseCubemap,
	uint32_t numMipmap,
	uint32_t inputCubeSideLength)
{
	outputDiffuseCubemap->CreateImage(
		ctx,
		inputCubeSideLength,
		inputCubeSideLength,
		numMipmap,
		IBLConfig::LayerCount,
		IBLConfig::CubeFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	outputDiffuseCubemap->CreateImageView(
		ctx,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		IBLConfig::LayerCount,
		numMipmap);
}

void PipelineCubeFilter::CreateDescriptor(VulkanContext& ctx, VulkanImage* inputCubemap)
{
	VulkanDescriptorInfo dsInfo;
	dsInfo.AddImage(inputCubemap);
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, 1u, 1u);
	descriptor_.CreateSet(ctx, dsInfo, &descriptorSet_);
}

void PipelineCubeFilter::CreateOutputCubemapViews(VulkanContext& ctx,
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
			VK_CHECK(vkCreateImageView(ctx.GetDevice(), &viewInfo, nullptr, &outputCubemapViews[a][b]));
		}
	}
}

void PipelineCubeFilter::CreateOffscreenGraphicsPipeline(
	VulkanContext& ctx,
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
		VK_CHECK(shaderModules[i].Create(ctx.GetDevice(), file));
		VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Pipeline create info
	PipelineCreateInfo pInfo(ctx);
	pInfo.viewport.width = static_cast<float>(viewportWidth);
	pInfo.viewport.height = static_cast<float>(viewportHeight);
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
		ctx.GetDevice(),
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		pipeline));

	for (auto s : shaderModules)
	{
		s.Destroy();
	}
}

void PipelineCubeFilter::OffscreenRender(VulkanContext& ctx,
	VulkanImage* outputCubemap,
	CubeFilterType filterType)
{
	const uint32_t outputMipMapCount = filterType == CubeFilterType::Diffuse ?
		1u :
		Utility::MipMapCount(IBLConfig::OutputSpecularSideLength);

	const uint32_t outputSideLength = filterType == CubeFilterType::Diffuse ?
		IBLConfig::OutputDiffuseSideLength :
		IBLConfig::OutputSpecularSideLength;

	InitializeOutputCubemap(ctx, outputCubemap, outputMipMapCount, outputSideLength);

	// Create views from the output cubemap
	std::vector<std::vector<VkImageView>> outputViews;
	CreateOutputCubemapViews(ctx, outputCubemap, outputViews, outputMipMapCount);

	// Framebuffers
	std::vector<VulkanFramebuffer> mipFramebuffers(outputMipMapCount);
	for (uint32_t i = 0; i < outputMipMapCount; ++i)
	{
		const uint32_t targetSize = outputSideLength >> i;
		mipFramebuffers[i].CreateUnresizeable(ctx, renderPass_.GetHandle(), outputViews[i], targetSize, targetSize);
	}

	// Get command buffers
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();

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
	VkPipeline pipeline = graphicsPipelines_[static_cast<uint8_t>(filterType)];

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VulkanImage::TransitionLayoutCommand(commandBuffer,
		outputCubemap->image_,
		outputCubemap->imageFormat_,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		// Transition the entire mip levels and layers
		0u, 
		outputCubemap->mipCount_,
		0u, 
		outputCubemap->layerCount_);

	for (int i = static_cast<int>(outputMipMapCount - 1u); i >= 0; --i)
	{
		const uint32_t targetSize = outputSideLength >> i;

		PushConstCubeFilter pc =
		{
			.roughness = filterType == CubeFilterType::Diffuse || outputMipMapCount == 1 ?
				0.f :
				static_cast<float>(i) / static_cast<float>(outputMipMapCount - 1),
			.outputDiffuseSampleCount = IBLConfig::OutputDiffuseSampleCount
		};
		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout_,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PushConstCubeFilter), &pc);

		renderPass_.BeginCubemapRenderPass(commandBuffer, mipFramebuffers[i].GetFramebuffer(), targetSize);
		vkCmdDraw(commandBuffer, 3, 1u, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
	}

	// Convention is to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	VulkanImage::TransitionLayoutCommand(commandBuffer,
		outputCubemap->image_,
		outputCubemap->imageFormat_,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		// Transition the entire mip levels and layers
		0u,
		outputCubemap->mipCount_,
		0u,
		outputCubemap->layerCount_);

	ctx.EndOneTimeGraphicsCommand(commandBuffer);

	// Destroy frame buffers
	for (auto& f : mipFramebuffers)
	{
		f.Destroy();
	}

	// Destroy image views
	for (auto& views : outputViews)
	{
		for (VkImageView& view : views)
		{
			vkDestroyImageView(ctx.GetDevice(), view, nullptr);
		}
	}

	// Create a sampler for the output cubemap
	outputCubemap->CreateDefaultSampler(
		ctx,
		0.0f,
		static_cast<float>(outputMipMapCount));
}