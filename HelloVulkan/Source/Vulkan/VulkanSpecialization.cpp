#include "VulkanSpecialization.h"

void VulkanSpecialization::ConsumeEntries(
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

void VulkanSpecialization::Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderInfoArray)
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