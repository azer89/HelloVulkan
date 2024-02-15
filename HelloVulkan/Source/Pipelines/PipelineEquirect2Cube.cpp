#include "PipelineEquirect2Cube.h"
#include "PipelineCreateInfo.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "Configs.h"

PipelineEquirect2Cube::PipelineEquirect2Cube(
	VulkanDevice& vkDev, 
	const std::string& hdrFile) :
	PipelineBase(
		vkDev, 
		{
			.type_ = PipelineType::GraphicsOffScreen
		}
	)
{
	InitializeHDRImage(vkDev, hdrFile);
	renderPass_.CreateOffScreenCubemapRenderPass(vkDev, IBLConfig::CubeFormat);

	CreateDescriptor(vkDev);

	CreatePipelineLayout(vkDev, descriptor_.layout_, &pipelineLayout_);

	CreateOffscreenGraphicsPipeline(
		vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "FullscreenTriangle.vert",
			AppConfig::ShaderFolder + "Equirect2Cube.frag"
		},
		&pipeline_
	);
}

PipelineEquirect2Cube::~PipelineEquirect2Cube()
{
	inputHDRImage_.Destroy();
	vkDestroyFramebuffer(device_, cubeFramebuffer_, nullptr);
}

void PipelineEquirect2Cube::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
}

void PipelineEquirect2Cube::InitializeCubemap(VulkanDevice& vkDev, VulkanImage* cubemap)
{
	uint32_t mipmapCount = Utility::MipMapCount(IBLConfig::InputCubeSideLength);

	cubemap->CreateImage(
		vkDev,
		IBLConfig::InputCubeSideLength,
		IBLConfig::InputCubeSideLength,
		mipmapCount,
		IBLConfig::LayerCount,
		IBLConfig::CubeFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	cubemap->CreateImageView(
		vkDev,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		IBLConfig::LayerCount,
		mipmapCount);
}

void PipelineEquirect2Cube::InitializeHDRImage(VulkanDevice& vkDev, const std::string& hdrFile)
{
	inputHDRImage_.CreateFromHDR(vkDev, hdrFile.c_str());
	inputHDRImage_.CreateImageView(
		vkDev,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT);
	inputHDRImage_.CreateDefaultSampler(
		vkDev,
		0.f,
		1.f);
}

void PipelineEquirect2Cube::CreateDescriptor(VulkanDevice& vkDev)
{
	// Pool
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.frameCount_ = 1u,
			.setCountPerFrame_ = 1u,
			.flags_ = 0
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
	VkDescriptorImageInfo imageInfo = inputHDRImage_.GetDescriptorImageInfo();

	descriptor_.CreateSet(
		vkDev,
		{
			{.imageInfoPtr_ = &imageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }
		},
		&descriptorSet_);
}

void PipelineEquirect2Cube::CreateOffscreenGraphicsPipeline(
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

	pInfo.viewport.width = IBLConfig::InputCubeSideLength;
	pInfo.viewport.height = IBLConfig::InputCubeSideLength;

	pInfo.scissor.extent = { IBLConfig::InputCubeSideLength, IBLConfig::InputCubeSideLength };

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

// TODO Can be moved to generic function in PipelineBase
void PipelineEquirect2Cube::CreateFrameBuffer(
	VulkanDevice& vkDev, 
	std::vector<VkImageView> outputViews)
{
	VkFramebufferCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.renderPass = renderPass_.GetHandle(),
		.attachmentCount = static_cast<uint32_t>(outputViews.size()),
		.pAttachments = outputViews.data(),
		.width = IBLConfig::InputCubeSideLength,
		.height = IBLConfig::InputCubeSideLength,
		.layers = 1u,
	};

	VK_CHECK(vkCreateFramebuffer(vkDev.GetDevice(), &info, nullptr, &cubeFramebuffer_));
}

// TODO Move this to VulkanImage
void PipelineEquirect2Cube::CreateCubemapViews(
	VulkanDevice& vkDev, 
	VulkanImage* cubemap,
	std::vector<VkImageView>& cubemapViews)
{
	cubemapViews = std::vector<VkImageView>(IBLConfig::LayerCount, VK_NULL_HANDLE);
	for (size_t i = 0; i < IBLConfig::LayerCount; i++)
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

void PipelineEquirect2Cube::OffscreenRender(VulkanDevice& vkDev, VulkanImage* outputEnvMap)
{
	// Initialize output cubemap
	InitializeCubemap(vkDev, outputEnvMap);

	// Create views from the output cubemap
	std::vector<VkImageView> outputViews;
	CreateCubemapViews(vkDev, outputEnvMap, outputViews);

	CreateFrameBuffer(vkDev, outputViews);

	VkCommandBuffer commandBuffer = vkDev.BeginOneTimeGraphicsCommand();

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

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

	renderPass_.BeginCubemapRenderPass(commandBuffer, cubeFramebuffer_, IBLConfig::InputCubeSideLength);

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

	vkDev.EndOneTimeGraphicsCommand(commandBuffer);

	// Create a sampler for the output cubemap
	outputEnvMap->CreateDefaultSampler(vkDev);

	// Destroy image views
	for (size_t i = 0; i < IBLConfig::LayerCount; i++)
	{
		vkDestroyImageView(vkDev.GetDevice(), outputViews[i], nullptr);
	}
}