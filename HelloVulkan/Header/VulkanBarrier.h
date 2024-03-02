#ifndef VULKAN_BARRIER
#define VULKAN_BARRIER

#include "volk.h"

#include <vector>

// TODO this abtraction is still fairly simple
class VulkanBarrier
{
public:
	static void CreateMemoryBarrier(
		VkCommandBuffer commandBuffer,
		const VkMemoryBarrier2* barriers,
		uint32_t barrierCount
	)
	{
		VkDependencyInfo dependencyInfo = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.memoryBarrierCount = barrierCount,
			.pMemoryBarriers = barriers
		};
		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	static void CreateImageBarrier(
		VkCommandBuffer commandBuffer,
		const VkImageMemoryBarrier2* barriers,
		uint32_t barrierCount
	)
	{
		VkDependencyInfo depInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr ,
			.imageMemoryBarrierCount = barrierCount,
			.pImageMemoryBarriers = barriers
		};
		vkCmdPipelineBarrier2(commandBuffer, &depInfo);
	}

	static void CreateBufferBarrier(
		VkCommandBuffer commandBuffer,
		const VkBufferMemoryBarrier2* barriers,
		uint32_t barrierCount
	)
	{
		VkDependencyInfo dependencyInfo = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.bufferMemoryBarrierCount = barrierCount,
			.pBufferMemoryBarriers = barriers
		};
		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}
};

#endif