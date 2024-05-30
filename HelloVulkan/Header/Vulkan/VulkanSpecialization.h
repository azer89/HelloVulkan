#ifndef VULKAN_SPECIALIZATION_CONSTANTS
#define VULKAN_SPECIALIZATION_CONSTANTS

#include "volk.h"

#include <vector>

class VulkanSpecialization
{
public:
	VulkanSpecialization() = default;

private:
	std::vector<VkSpecializationMapEntry> entries_{};
	VkSpecializationInfo specializationInfo_{};
	VkShaderStageFlags shaderStages_;

public:
	void ConsumeEntries(
		std::vector<VkSpecializationMapEntry>&& entries,
		void* data,
		size_t dataSize,
		VkShaderStageFlags shaderStages);
	void Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderInfoArray);
};

#endif