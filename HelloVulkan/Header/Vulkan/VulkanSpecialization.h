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
	VkShaderStageFlags shaderStages_;

public:
	void ConsumeEntries(
		std::vector<VkSpecializationMapEntry>&& entries,
		void* data,
		size_t dataSize,
		VkShaderStageFlags shaderStages)
	{
		entries_ = std::move(entries);

		specializationInfo_.dataSize = dataSize;
		specializationInfo_.mapEntryCount = static_cast<uint32_t>(entries_.size());
		specializationInfo_.pMapEntries = entries_.data();
		specializationInfo_.pData = data;

		shaderStages_ = shaderStages;
	}

	void Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderInfoArray)
	{
		if (entries_.empty())
		{
			return;
		}

		for (auto& stageInfo : shaderInfoArray)
		{
			if (stageInfo.stage & shaderStages_)
			{
				stageInfo.pSpecializationInfo = &specializationInfo_;
				break;
			}
		}
	}
};

#endif