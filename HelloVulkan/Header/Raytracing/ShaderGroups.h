#ifndef RAYTRACING_SHADER_GROUPS
#define RAYTRACING_SHADER_GROUPS

#include "volk.h"

#include <vector>

struct ShaderGroups
{
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups_;

	VkRayTracingShaderGroupCreateInfoKHR* Data() { return shaderGroups_.data(); }

	uint32_t Count() { return static_cast<uint32_t>(shaderGroups_.size()); }

	void Create()
	{
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
	}
};

#endif