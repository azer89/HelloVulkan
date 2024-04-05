#include "PipelineSimpleRaytracing.h"
#include "RaytracingBuilder.h"
#include "VulkanShader.h"
#include "VulkanCheck.h"
#include "Scene.h"
#include "Configs.h"
#include "Utility.h"

PipelineSimpleRaytracing::PipelineSimpleRaytracing(VulkanContext& ctx, Scene* scene, ResourcesLight* resourcesLight) :
	PipelineBase(
		ctx,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}),
	scene_(scene),
	resourcesLight_(resourcesLight)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(RaytracingCameraUBO), AppConfig::FrameCount);

	CreateBLAS(ctx);
	CreateTLAS(ctx);
	CreateStorageImage(ctx);
	CreateDescriptor(ctx);
	sg_.Create();
	CreateRayTracingPipeline(ctx);
	sbt_.Create(ctx, pipeline_, sg_.Count());
}

PipelineSimpleRaytracing::~PipelineSimpleRaytracing()
{
	storageImage_.Destroy();
	blas_.Destroy();
	tlas_.Destroy();
	for (auto& mData : modelDataArray_)
	{
		mData.Destroy();
	}
	sbt_.Destroy();
}

void PipelineSimpleRaytracing::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
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
		&sbt_.raygenShaderSbtEntry_,
		&sbt_.missShaderSbtEntry_,
		&sbt_.hitShaderSbtEntry_,
		&sbt_.callableShaderSbtEntry_,
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

void PipelineSimpleRaytracing::OnWindowResized(VulkanContext& ctx)
{
	storageImage_.Destroy();
	CreateStorageImage(ctx);
	UpdateDescriptor(ctx);
}

void PipelineSimpleRaytracing::CreateDescriptor(VulkanContext& ctx)
{
	textureInfoArray_ = scene_->GetImageInfos();
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	descriptorInfo_.AddAccelerationStructure();
	descriptorInfo_.AddImage(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	descriptorInfo_.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	descriptorInfo_.AddBuffer(&(scene_->vertexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	descriptorInfo_.AddBuffer(&(scene_->indexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	descriptorInfo_.AddBuffer(&(scene_->meshDataBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	descriptorInfo_.AddBuffer(resourcesLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	descriptorInfo_.AddImageArray(textureInfoArray_, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

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
void PipelineSimpleRaytracing::UpdateDescriptor(VulkanContext& ctx)
{
	VkWriteDescriptorSetAccelerationStructureKHR asInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.accelerationStructureCount = 1u,
		.pAccelerationStructures = &tlas_.handle_,
	};
	descriptorInfo_.UpdateAccelerationStructure(&asInfo, 0);

	descriptorInfo_.UpdateStorageImage(&storageImage_, 1);

	constexpr auto frameCount = AppConfig::FrameCount;
	for (size_t i = 0; i < frameCount; i++)
	{
		descriptorInfo_.UpdateBuffer(&(cameraUBOBuffers_[i]), 2);
		descriptor_.UpdateSet(ctx, descriptorInfo_, &(descriptorSets_[i]));
	}
}

void PipelineSimpleRaytracing::CreateStorageImage(VulkanContext& ctx)
{
	storageImage_.CreateImage(
		ctx,
		ctx.GetSwapchainWidth(),
		ctx.GetSwapchainHeight(),
		1u,
		1u,
		ctx.GetSwapchainImageFormat(),
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY
	);

	storageImage_.CreateImageView(
		ctx,
		ctx.GetSwapchainImageFormat(),
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u,
		0u,
		1u);

	storageImage_.TransitionLayout(ctx, 
		storageImage_.imageFormat_, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_GENERAL);
}

void PipelineSimpleRaytracing::CreateRayTracingPipeline(VulkanContext& ctx)
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
		AppConfig::ShaderFolder + "Raytracing/RayGen.rgen",
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
		.groupCount = sg_.Count(),
		.pGroups = sg_.Data(),
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

void PipelineSimpleRaytracing::CreateBLAS(VulkanContext& ctx)
{
	RaytracingBuilder::CreateRTModelDataArray(ctx, scene_, modelDataArray_);

	RaytracingBuilder::CreateBLASMultipleMeshes(ctx, modelDataArray_, &blas_);
}

void PipelineSimpleRaytracing::CreateTLAS(VulkanContext& ctx)
{
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };

	RaytracingBuilder::CreateTLAS(ctx, transformMatrix, blas_.deviceAddress_, &tlas_);
}