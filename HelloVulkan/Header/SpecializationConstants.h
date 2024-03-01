#ifndef SPECIALIZATION_CONSTANTS
#define SPECIALIZATION_CONSTANTS

#include "volk.h"

#include <vector>

class SpecializationConstants
{
public:
	SpecializationConstants() : 
		specializationInfo_({}),
		created_(false)
	{
	}

private:
	VkSpecializationInfo specializationInfo_;
	VkShaderStageFlagBits shaderStage_;
	bool created_;

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

	// TODO currently can only inject one shader stage
	void Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
	{
		if (!created_)
		{
			return;
		}

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