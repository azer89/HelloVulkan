#ifndef RAYTRACING_STRUCTS
#define RAYTRACING_STRUCTS

#include "VulkanBuffer.h"

struct ScratchBuffer
{
	// TODO VMA
	uint64_t deviceAddress_ = 0;
	VkBuffer handle_ = VK_NULL_HANDLE;
	VkDeviceMemory memory_ = VK_NULL_HANDLE;

	void Destroy(VkDevice device)
	{
		vkFreeMemory(device, memory_, nullptr);
	}
};

// Ray tracing acceleration structure
struct AccelerationStructure
{
	// TODO VMA
	VkAccelerationStructureKHR handle_;
	uint64_t deviceAddress_ = 0;
	VkDeviceMemory memory_;
	VkBuffer buffer_;


	void Destroy(VkDevice device)
	{
		vkFreeMemory(device, memory_, nullptr);
		vkDestroyBuffer(device, buffer_, nullptr);
	}
};

#endif