#include "PipelineRaytracing.h"
#include "RaytracingBuilder.h"
#include "VulkanShader.h"
#include "VulkanCheck.h"
#include "Scene.h"
#include "Configs.h"
#include "Utility.h"

PipelineRaytracing::PipelineRaytracing(VulkanContext& ctx, Scene* scene) :
	PipelineBase(
		ctx,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}),
	scene_(scene)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, rtUBOBuffers_, sizeof(RaytracingUBO), AppConfig::FrameCount);

	CreateBLAS(ctx);
	CreateTLAS(ctx);
	CreateBDABuffer(ctx); // Buffer device address
	CreateStorageImage(ctx, ctx.GetSwapchainImageFormat(), &storageImage_);
	CreateStorageImage(ctx, VK_FORMAT_R32G32B32A32_SFLOAT, &accumulationImage_);
	CreateDescriptor(ctx);
	shaderGroups_.Create();
	CreateRayTracingPipeline(ctx);
	shaderBindingTables_.Create(ctx, pipeline_, shaderGroups_.Count());
}

PipelineRaytracing::~PipelineRaytracing()
{
	blas_.Destroy();
	tlas_.Destroy();
	bdaBuffer_.Destroy();
	storageImage_.Destroy();
	accumulationImage_.Destroy();
	shaderBindingTables_.Destroy();
	for (auto& mData : modelDataArray_)
	{
		mData.Destroy();
	}
	for (auto& buffer : rtUBOBuffers_)
	{
		buffer.Destroy();
	}
}

void PipelineRaytracing::OnWindowResized(VulkanContext& ctx)
{
	storageImage_.Destroy();
	accumulationImage_.Destroy();
	CreateStorageImage(ctx, ctx.GetSwapchainImageFormat(), &storageImage_);
	CreateStorageImage(ctx, VK_FORMAT_R32G32B32A32_SFLOAT, &accumulationImage_);
	UpdateDescriptor(ctx);
	ResetFrameCounter();
}

void PipelineRaytracing::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Simple_Raytracing", tracy::Color::Orange1);

	const uint32_t frameIndex = ctx.GetFrameIndex();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_);
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
		pipelineLayout_, 
		0, 
		1, 
		&descriptorSets_[frameIndex], 0, 0);

	vkCmdTraceRaysKHR(
		commandBuffer,
		&shaderBindingTables_.raygenShaderSbtEntry_,
		&shaderBindingTables_.missShaderSbtEntry_,
		&shaderBindingTables_.hitShaderSbtEntry_,
		&shaderBindingTables_.callableShaderSbtEntry_,
		ctx.GetSwapchainWidth(),
		ctx.GetSwapchainHeight(),
		1);

	const uint32_t swapchainIndex = ctx.GetCurrentSwapchainImageIndex();

	VulkanImage::TransitionLayoutCommand(
		commandBuffer,
		storageImage_.image_,
		storageImage_.imageFormat_,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	VulkanImage::TransitionLayoutCommand(
		commandBuffer, 
		ctx.GetSwapchainImage(swapchainIndex),
		ctx.GetSwapchainImageFormat(),
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	
	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { ctx.GetSwapchainWidth(), ctx.GetSwapchainHeight(), 1 };
	vkCmdCopyImage(commandBuffer, 
		storageImage_.image_, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
		ctx.GetSwapchainImage(swapchainIndex), 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		1, 
		&copyRegion);
	
	VulkanImage::TransitionLayoutCommand(
		commandBuffer,
		ctx.GetSwapchainImage(swapchainIndex),
		ctx.GetSwapchainImageFormat(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VulkanImage::TransitionLayoutCommand(commandBuffer,
		storageImage_.image_,
		storageImage_.imageFormat_,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL);
}

void PipelineRaytracing::CreateDescriptor(VulkanContext& ctx)
{
	textureInfoArray_ = scene_->GetImageInfos();
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	descriptorInfo_.AddAccelerationStructure(VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	descriptorInfo_.AddImage(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	descriptorInfo_.AddImage(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	descriptorInfo_.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
	descriptorInfo_.AddBuffer(&bdaBuffer_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	descriptorInfo_.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	descriptorInfo_.AddImageArray(textureInfoArray_, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);

	// Create pool and layout
	descriptor_.CreatePoolAndLayout(ctx, descriptorInfo_, frameCount, 1u);

	for (size_t i = 0; i < frameCount; i++)
	{
		descriptor_.AllocateSet(ctx, &(descriptorSets_[i]));
	}

	// Rebuild descriptor sets
	UpdateDescriptor(ctx);
}

// Rebuild the entire descriptor sets
void PipelineRaytracing::UpdateDescriptor(VulkanContext& ctx)
{
	VkWriteDescriptorSetAccelerationStructureKHR asInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.accelerationStructureCount = 1u,
		.pAccelerationStructures = &tlas_.handle_,
	};
	descriptorInfo_.UpdateAccelerationStructure(&asInfo, 0);

	// TODO Currently storage images need to be readded
	descriptorInfo_.UpdateStorageImage(&storageImage_, 1);
	descriptorInfo_.UpdateStorageImage(&accumulationImage_, 2);

	constexpr auto frameCount = AppConfig::FrameCount;
	for (size_t i = 0; i < frameCount; i++)
	{
		descriptorInfo_.UpdateBuffer(&(rtUBOBuffers_[i]), 3);
		descriptorInfo_.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 5);
		descriptor_.UpdateSet(ctx, descriptorInfo_, &(descriptorSets_[i]));
	}
}

void PipelineRaytracing::CreateRayTracingPipeline(VulkanContext& ctx)
{
	// Pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutCI =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptor_.layout_
	};
	VK_CHECK(vkCreatePipelineLayout(ctx.GetDevice(), &pipelineLayoutCI, nullptr, &pipelineLayout_));

	// Shaders
	const std::vector<std::string> shaderFiles =
	{
		AppConfig::ShaderFolder + "Raytracing/RayGeneration.rgen",
		AppConfig::ShaderFolder + "Raytracing/Miss.rmiss",
		AppConfig::ShaderFolder + "Raytracing/Shadow.rmiss",
		AppConfig::ShaderFolder + "Raytracing/ClosestHit.rchit",
		AppConfig::ShaderFolder + "Raytracing/AnyHit.rahit"
	};

	std::vector<VulkanShader> shaderModules(shaderFiles.size());
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaderFiles.size());
	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i].c_str();
		VK_CHECK(shaderModules[i].Create(ctx.GetDevice(), file));
		const VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Pipeline
	const VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI =
	{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.groupCount = shaderGroups_.Count(),
		.pGroups = shaderGroups_.Data(),
		.maxPipelineRayRecursionDepth = 1,
		.layout = pipelineLayout_
	};
	VK_CHECK(vkCreateRayTracingPipelinesKHR(
		ctx.GetDevice(), 
		VK_NULL_HANDLE, 
		VK_NULL_HANDLE, 
		1, 
		&rayTracingPipelineCI, 
		nullptr, 
		&pipeline_));

	for (VulkanShader& s : shaderModules)
	{
		s.Destroy();
	}
}

void PipelineRaytracing::CreateStorageImage(VulkanContext& ctx, VkFormat imageFormat, VulkanImage* image)
{
	const uint32_t imageWidth = ctx.GetSwapchainWidth();
	const uint32_t imageHeight = ctx.GetSwapchainHeight();

	image->CreateImage(
		ctx,
		imageWidth,
		imageHeight,
		1u,
		1u,
		imageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY
	);

	image->CreateImageView(
		ctx,
		imageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u,
		0u,
		1u);

	image->TransitionLayout(ctx,
		imageFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL);
}

void PipelineRaytracing::CreateBDABuffer(VulkanContext& ctx)
{
	BDA bda = scene_->GetBDA();
	VkDeviceSize bdaSize = sizeof(BDA);
	bdaBuffer_.CreateBuffer(
		ctx,
		bdaSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	bdaBuffer_.UploadBufferData(ctx, &bda, bdaSize);
}

// Bottom Level Acceleration Structure
void PipelineRaytracing::CreateBLAS(VulkanContext& ctx)
{
	RaytracingBuilder::CreateRTModelDataArray(ctx, scene_, modelDataArray_);
	RaytracingBuilder::CreateBLASMultipleMeshes(ctx, modelDataArray_, &blas_);
}

// Top Level Acceleration Structure
void PipelineRaytracing::CreateTLAS(VulkanContext& ctx)
{
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };
	RaytracingBuilder::CreateTLAS(ctx, transformMatrix, blas_.deviceAddress_, &tlas_);
}

void PipelineRaytracing::SetRaytracingUBO(
	VulkanContext& ctx,
	const glm::mat4& inverseProjection,
	const glm::mat4& inverseView,
	const glm::vec3& cameraPosition)
{
	ubo_.projectionInverse = inverseProjection;
	ubo_.viewInverse = inverseView;
	ubo_.cameraPosition = glm::vec4(cameraPosition, 1.0);
	ubo_.frame = frameCounter_++;
	ubo_.currentSampleCount += ubo_.sampleCountPerFrame;

	const uint32_t frameIndex = ctx.GetFrameIndex();
	rtUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo_, sizeof(RaytracingUBO));
}