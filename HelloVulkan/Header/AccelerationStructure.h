#ifndef ACCELERATION_STRUCTURE
#define ACCELERATION_STRUCTURE

#include "VulkanBuffer.h"
#include "vk_mem_alloc.h"

class AccelerationStructure
{
public:
	// TODO Create a function to create handle_
	VkAccelerationStructureKHR handle_;

	uint64_t deviceAddress_ = 0;
	VkBuffer buffer_;
	VmaAllocator vmaAllocator_;
	VmaAllocation vmaAllocation_;
	VmaAllocationInfo vmaInfo_;
	VkDevice device_;

	AccelerationStructure() :
		handle_(nullptr),
		deviceAddress_(0),
		buffer_(nullptr),
		vmaAllocator_(nullptr),
		vmaAllocation_(nullptr),
		vmaInfo_({}),
		device_(nullptr)
	{
	}

	void Create(
		VulkanDevice& vkDev,
		VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

public:
	void Destroy();
};

#endif