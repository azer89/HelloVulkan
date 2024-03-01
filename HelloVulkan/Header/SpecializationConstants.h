#ifndef SPECIALIZATION_CONSTANTS
#define SPECIALIZATION_CONSTANTS

#include "volk.h"

#include <vector>

class SpecializationConstants
{
private:
	VkSpecializationInfo specializationInfo_{};
	VkShaderStageFlagBits shaderStage_;

public:
	void Create(
		std::vector<VkSpecializationMapEntry> entries, 
		void* data, 
		size_t dataSize, 
		VkShaderStageFlagBits shaderStage)
	{
		shaderStage_ = shaderStage;

		specializationInfo_.dataSize = dataSize;
		specializationInfo_.mapEntryCount = static_cast<uint32_t>(entries.size());
		specializationInfo_.pMapEntries = entries.data();
		specializationInfo_.pData = data;
	}

	void Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
	{
		for (auto& stage : shaderStages)
		{
			if (stage.stage == shaderStage_)
			{
				stage.pSpecializationInfo = &specializationInfo_;
				break;
			}
		}
	}

};

#endif