#ifndef RAYTRACING_SHADER_GROUPS
#define RAYTRACING_SHADER_GROUPS

#include "volk.h"

#include <vector>

struct ShaderGroups
{
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups_;

	VkRayTracingShaderGroupCreateInfoKHR* Data() { return shaderGroups_.data(); }

	uint32_t Count() const { return static_cast<uint32_t>(shaderGroups_.size()); }

	void Create()
	{
		shaderGroups_ =
		{
			// Ray Generation
			{
				.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
				.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
				.generalShader = 0u,
				.closestHitShader = VK_SHADER_UNUSED_KHR,
				.anyHitShader = VK_SHADER_UNUSED_KHR,
				.intersectionShader = VK_SHADER_UNUSED_KHR
			},
			// Miss
			{
				.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
				.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
				.generalShader = 1u,
				.closestHitShader = VK_SHADER_UNUSED_KHR,
				.anyHitShader = VK_SHADER_UNUSED_KHR,
				.intersectionShader = VK_SHADER_UNUSED_KHR,
			},
			// Shadow
			{
				.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
				.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
				.generalShader = 2u,
				.closestHitShader = VK_SHADER_UNUSED_KHR,
				.anyHitShader = VK_SHADER_UNUSED_KHR,
				.intersectionShader = VK_SHADER_UNUSED_KHR,
			},
			// Closest Hit and Any Hit
			{
				.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
				.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
				.generalShader = VK_SHADER_UNUSED_KHR,
				.closestHitShader = 3u,
				.anyHitShader = 4u,
				.intersectionShader = VK_SHADER_UNUSED_KHR
			}
		};
	}
};

#endif