#include "PipelineSimpleRaytracing.h"
#include "RaytracingBuilder.h"
#include "VulkanShader.h"
#include "VulkanCheck.h"
#include "Scene.h"
#include "Configs.h"
#include "Utility.h"

PipelineSimpleRaytracing::PipelineSimpleRaytracing(VulkanContext& ctx, Scene* scene) :
	PipelineBase(
		ctx,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}),
	scene_(scene)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(RaytracingCameraUBO), AppConfig::FrameCount);

	CreateBLAS(ctx);
	CreateTLAS(ctx);

	CreateStorageImage(ctx);
	CreateDescriptor(ctx);
	CreateRayTracingPipeline(ctx);
	CreateShaderBindingTable(ctx);
}

PipelineSimpleRaytracing::~PipelineSimpleRaytracing()
{
	storageImage_.Destroy();
	blas_.Destroy();
	tlas_.Destroy();
	vertexBuffer_.Destroy();
	indexBuffer_.Destroy();
	transformBuffer_.Destroy();
	raygenShaderBindingTable_.Destroy();
	missShaderBindingTable_.Destroy();
	hitShaderBindingTable_.Destroy();
}

void PipelineSimpleRaytracing::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Simple_Raytracing", tracy::Color::Orange1);

	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties = ctx.GetRayTracingPipelineProperties();
	const uint32_t handleSizeAligned = Utility::AlignedSize(properties.shaderGroupHandleSize, properties.shaderGroupHandleAlignment);

	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry =
	{
		.deviceAddress = raygenShaderBindingTable_.deviceAddress_,
		.stride = handleSizeAligned,
		.size = handleSizeAligned
	};

	const VkStridedDeviceAddressRegionKHR missShaderSbtEntry =
	{
		.deviceAddress = missShaderBindingTable_.deviceAddress_,
		.stride = handleSizeAligned,
		.size = handleSizeAligned,
	};

	const VkStridedDeviceAddressRegionKHR hitShaderSbtEntry =
	{
		.deviceAddress = hitShaderBindingTable_.deviceAddress_,
		.stride = handleSizeAligned,
		.size = handleSizeAligned
	};
	const VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

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
		&raygenShaderSbtEntry,
		&missShaderSbtEntry,
		&hitShaderSbtEntry,
		&callableShaderSbtEntry,
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
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	descriptorInfo_.AddAccelerationStructure();
	descriptorInfo_.AddImage(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	descriptorInfo_.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	descriptorInfo_.AddBuffer(&(scene_->vertexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	descriptorInfo_.AddBuffer(&(scene_->indexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

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
		AppConfig::ShaderFolder + "SimpleRaytracing/RayGen.rgen",
		AppConfig::ShaderFolder + "SimpleRaytracing/Miss.rmiss",
		AppConfig::ShaderFolder + "SimpleRaytracing/ClosestHit.rchit"
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

	shaderGroups_ =
	{
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = 0u,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR
		},
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = 1u,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR,
		},
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
			.generalShader = VK_SHADER_UNUSED_KHR,
			.closestHitShader = 2u,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR
		}
	};

	// Pipeline
	const VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI =
	{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.groupCount = static_cast<uint32_t>(shaderGroups_.size()),
		.pGroups = shaderGroups_.data(),
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
	// Setup identity transform matrix
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};
	
	uint32_t triangleCount = static_cast<uint32_t>(scene_->sceneData_.indices_.size()) / 3;
	uint32_t vertexCount = static_cast<uint32_t>(scene_->sceneData_.vertices_.size());
	VkDeviceSize vertexStride = sizeof(VertexData);
	
	vertexBuffer_.CreateBufferWithShaderDeviceAddress(
		ctx,
		scene_->sceneData_.vertices_.size() * sizeof(VertexData),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	vertexBuffer_.UploadBufferData(ctx, scene_->sceneData_.vertices_.data(), scene_->sceneData_.vertices_.size() * sizeof(VertexData));

	indexBuffer_.CreateBufferWithShaderDeviceAddress(
		ctx,
		scene_->sceneData_.indices_.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	indexBuffer_.UploadBufferData(ctx, scene_->sceneData_.indices_.data(), scene_->sceneData_.indices_.size() * sizeof(uint32_t));

	transformBuffer_.CreateBufferWithShaderDeviceAddress(
		ctx,
		sizeof(VkTransformMatrixKHR),
	VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	transformBuffer_.UploadBufferData(ctx, &transformMatrix, sizeof(VkTransformMatrixKHR));

	RaytracingBuilder::CreateBLAS(
		ctx,
		vertexBuffer_,
		indexBuffer_,
		transformBuffer_,
		triangleCount,
		vertexCount,
		vertexStride,
		&blas_);
}

void PipelineSimpleRaytracing::CreateTLAS(VulkanContext& ctx)
{
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };

	RaytracingBuilder::CreateTLAS(ctx, transformMatrix, blas_.deviceAddress_, &tlas_);
}

void PipelineSimpleRaytracing::CreateShaderBindingTable(VulkanContext& ctx)
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties = ctx.GetRayTracingPipelineProperties();
	const uint32_t handleSizeAligned = Utility::AlignedSize(properties.shaderGroupHandleSize, properties.shaderGroupHandleAlignment);

	const uint32_t handleSize = properties.shaderGroupHandleSize;
	const uint32_t groupCount = static_cast<uint32_t>(shaderGroups_.size());
	const uint32_t sbtSize = groupCount * handleSizeAligned;

	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(
		ctx.GetDevice(), 
		pipeline_, 
		0, 
		groupCount, 
		sbtSize, 
		shaderHandleStorage.data()));

	const VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
	const VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	raygenShaderBindingTable_.CreateBufferWithShaderDeviceAddress(ctx, handleSize, bufferUsage, memoryUsage);
	missShaderBindingTable_.CreateBufferWithShaderDeviceAddress(ctx, handleSize, bufferUsage, memoryUsage);
	hitShaderBindingTable_.CreateBufferWithShaderDeviceAddress(ctx, handleSize, bufferUsage, memoryUsage);

	// Copy handles
	raygenShaderBindingTable_.UploadBufferData(ctx, shaderHandleStorage.data(), handleSize);
	missShaderBindingTable_.UploadBufferData(ctx, shaderHandleStorage.data() + handleSizeAligned, handleSize);
	hitShaderBindingTable_.UploadBufferData(ctx, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
}