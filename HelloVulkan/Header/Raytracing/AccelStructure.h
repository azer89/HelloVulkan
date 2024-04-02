#ifndef RAYTRACING_ACCELERATION_STRUCTURE
#define RAYTRACING_ACCELERATION_STRUCTURE

#include "VulkanBuffer.h"
#include "vk_mem_alloc.h"

// For TLAS and BLAS
class AccelStructure
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

	AccelStructure() :
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
		VulkanContext& ctx,
		VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

public:
	void Destroy();
};

#endif