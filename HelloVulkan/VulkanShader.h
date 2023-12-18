#ifndef SHADER_MODULE
#define SHADER_MODULE

#include "volk.h"

#include "glslang_c_interface.h"
#include "resource_limits_c.h"

#include <vector>
#include <string>

VkShaderStageFlagBits GLSLangShaderStageToVulkan(glslang_stage_t sh);
glslang_stage_t GLSLangShaderStageFromFileName(const char* fileName);
inline int EndsWith(const char* s, const char* part);

class VulkanShader
{
public:
	VkShaderModule GetShaderModule() { return shaderModule; }

	VkResult Create(VkDevice device, const char* fileName);

	void Destroy(VkDevice device);

	VkPipelineShaderStageCreateInfo GetShaderStageInfo(
		VkShaderStageFlagBits shaderStage,
		const char* entryPoint);

private:
	std::vector<unsigned int> SPIRV;
	VkShaderModule shaderModule = nullptr;
	
private:
	size_t CompileShaderFile(const char* file);
	std::string ReadShaderFile(const char* fileName);
	size_t CompileShader(glslang_stage_t stage, const char* shaderSource);
	void PrintShaderSource(const char* text);
};

#endif
