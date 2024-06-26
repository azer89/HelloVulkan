#ifndef SHADER_MODULE
#define SHADER_MODULE

#include "volk.h"

#include <glslang/Include/glslang_c_interface.h>

#include <vector>
#include <string>

VkShaderStageFlagBits GetShaderStageFlagBits(const char* file);

/*
Adapted from
	3D Graphics Rendering Cookbook
	by Sergey Kosarevsky & Viktor Latypov
	github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook
*/

class VulkanShader
{
public:
	[[nodiscard]] VkShaderModule GetShaderModule() const { return shaderModule_; }

	VkResult Create(VkDevice device, const char* fileName);

	void Destroy();

	VkPipelineShaderStageCreateInfo GetShaderStageInfo(
		VkShaderStageFlagBits shaderStage,
		const char* entryPoint);

private:
	std::vector<unsigned int> spirv_{};
	VkShaderModule shaderModule_{};
	VkDevice device_{};

private:
	[[nodiscard]] size_t CompileShaderFile(const char* file);
	[[nodiscard]] std::string ReadShaderFile(const char* fileName);
	[[nodiscard]] size_t CompileShader(glslang_stage_t stage, const char* shaderSource);
	void PrintShaderSource(const char* text);
};

#endif
