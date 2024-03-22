#ifndef VULKAN_SPECIALIZATION_CONSTANTS
#define VULKAN_SPECIALIZATION_CONSTANTS

#include "volk.h"

#include <vector>

class VulkanSpecialization
{
public:
	VulkanSpecialization() = default;

private:
	std::vector<VkSpecializationMapEntry> entries_ = {};
	VkSpecializationInfo specializationInfo_ = {};
	VkShaderStageFlagBits shaderStage_;

public:
	void ConsumeEntries(
		std::vector<VkSpecializationMapEntry>&& entries,
		void* data,
		size_t dataSize,
		VkShaderStageFlagBits shaderStage)
	{
		entries_ = std::move(entries);

		specializationInfo_.dataSize = dataSize;
		specializationInfo_.mapEntryCount = static_cast<uint32_t>(entries_.size());
		specializationInfo_.pMapEntries = entries_.data();
		specializationInfo_.pData = data;

		shaderStage_ = shaderStage;
	}

	void Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
	{
		if (entries_.empty())
		{
			return;
		}

		for (auto& stage : shaderStages)
		{
			if (stage.stage & shaderStage_)
			{
				stage.pSpecializationInfo = &specializationInfo_;
				break;
			}
		}
	}
};

#endif