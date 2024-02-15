#ifndef ACCELERATION_STRUCTURE
#define ACCELERATION_STRUCTURE

#include "VulkanBuffer.h"
#include "vk_mem_alloc.h"

class AccelerationStructure
{
public:
	VkAccelerationStructureKHR handle_;
	uint64_t deviceAddress_ = 0;
	VkBuffer buffer_;
	VmaAllocator vmaAllocator_;
	VmaAllocation vmaAllocation_;
	VmaAllocationInfo vmaInfo_;

	void Create(
		VulkanDevice& vkDev,
		VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

public:
	void Destroy(VkDevice device);
};

#endif