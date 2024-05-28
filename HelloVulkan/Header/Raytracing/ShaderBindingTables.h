#ifndef RAYTRACING_SHADER_BINDING_TABLES
#define RAYTRACING_SHADER_BINDING_TABLES

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "VulkanCheck.h"
#include "Utility.h"

struct ShaderBindingTables
{
	VulkanBuffer raygenShaderBindingTable_;
	VulkanBuffer missShaderBindingTable_;
	VulkanBuffer hitShaderBindingTable_;

	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry_;
	VkStridedDeviceAddressRegionKHR missShaderSbtEntry_;
	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry_;
	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry_;

	void Destroy()
	{
		raygenShaderBindingTable_.Destroy();
		missShaderBindingTable_.Destroy();
		hitShaderBindingTable_.Destroy();
	}

	/*
	Group 1 : Raygen
	Group 2 : Miss + Shadow
	Group 3 : Closest Hit + Any Hit
	*/
	void Create(VulkanContext& ctx, VkPipeline pipeline, const uint32_t groupCount)
	{
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties = ctx.GetRayTracingPipelineProperties();
		const uint32_t handleSizeAligned = Utility::AlignedSize(properties.shaderGroupHandleSize, properties.shaderGroupHandleAlignment);

		const uint32_t handleSize = properties.shaderGroupHandleSize;
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(
			ctx.GetDevice(),
			pipeline,
			0,
			groupCount,
			sbtSize,
			shaderHandleStorage.data()));

		constexpr VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
		constexpr VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		raygenShaderBindingTable_.CreateBufferWithDeviceAddress(ctx, handleSize, bufferUsage, memoryUsage);
		missShaderBindingTable_.CreateBufferWithDeviceAddress(ctx, handleSize * 2, bufferUsage, memoryUsage);
		hitShaderBindingTable_.CreateBufferWithDeviceAddress(ctx, handleSize, bufferUsage, memoryUsage);

		// Copy handles
		raygenShaderBindingTable_.UploadBufferData(ctx, shaderHandleStorage.data(), handleSize);
		missShaderBindingTable_.UploadBufferData(ctx, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
		hitShaderBindingTable_.UploadBufferData(ctx, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);

		// Entries
		raygenShaderSbtEntry_ =
		{
			.deviceAddress = raygenShaderBindingTable_.deviceAddress_,
			.stride = handleSizeAligned,
			.size = handleSizeAligned
		};
		missShaderSbtEntry_ =
		{
			.deviceAddress = missShaderBindingTable_.deviceAddress_,
			.stride = handleSizeAligned,
			.size = handleSizeAligned,
		};
		hitShaderSbtEntry_ =
		{
			.deviceAddress = hitShaderBindingTable_.deviceAddress_,
			.stride = handleSizeAligned,
			.size = handleSizeAligned
		};
		callableShaderSbtEntry_{};
	}
};

#endif