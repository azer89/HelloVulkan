#ifndef VULKAN_SPECIALIZATION_CONSTANTS
#define VULKAN_SPECIALIZATION_CONSTANTS

#include "volk.h"

#include <vector>

class VulkanSpecialization
{
public:
	VulkanSpecialization() :
		specializationInfo_({}),
		created_(false)
	{
	}

private:
	VkSpecializationInfo specializationInfo_ = {};
	VkShaderStageFlagBits shaderStage_;
	bool created_ = false;

public:
	void Create(
		std::vector<VkSpecializationMapEntry>& entries,
		void* data,
		size_t dataSize,
		VkShaderStageFlagBits shaderStage)
	{
		specializationInfo_.dataSize = dataSize;
		specializationInfo_.mapEntryCount = static_cast<uint32_t>(entries.size());
		specializationInfo_.pMapEntries = entries.data();
		specializationInfo_.pData = data;

		shaderStage_ = shaderStage;
		created_ = true;
	}

	void Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
	{
		if (!created_)
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