#ifndef VULKAN_BARRIER
#define VULKAN_BARRIER

#include "volk.h"

struct ImageBarrierInfo
{
	VkCommandBuffer commandBuffer;
	
	VkImageLayout oldLayout;
	VkPipelineStageFlags2 sourceStage;
	VkAccessFlags2 sourceAccess;
	
	VkImageLayout newLayout;
	VkPipelineStageFlags2 destinationStage;
	VkAccessFlags2 destinationAccess;
};

// TODO this abstraction is still fairly simple
class VulkanBarrier
{
public:
	static void CreateMemoryBarrier(
		VkCommandBuffer commandBuffer,
		const VkMemoryBarrier2* barriers,
		uint32_t barrierCount
	)
	{
		const VkDependencyInfo dependencyInfo = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.memoryBarrierCount = barrierCount,
			.pMemoryBarriers = barriers
		};
		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	static void CreateBufferBarrier(
		VkCommandBuffer commandBuffer,
		const VkBufferMemoryBarrier2* barriers,
		uint32_t barrierCount
	)
	{
		const VkDependencyInfo dependencyInfo = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.bufferMemoryBarrierCount = barrierCount,
			.pBufferMemoryBarriers = barriers
		};
		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	static void CreateImageBarrier(const ImageBarrierInfo& info, const VkImageSubresourceRange& range, VkImage image)
	{
		const VkImageMemoryBarrier2 barrier =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = info.sourceStage,
			.srcAccessMask = info.sourceAccess,
			.dstStageMask = info.destinationStage,
			.dstAccessMask = info.destinationAccess,
			.oldLayout = info.oldLayout,
			.newLayout = info.newLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = range
		};
		CreateImageBarrier(info.commandBuffer, &barrier, 1u);
	}

	static void CreateImageBarrier(
		VkCommandBuffer commandBuffer,
		const VkImageMemoryBarrier2* barriers,
		uint32_t barrierCount
	)
	{
		const VkDependencyInfo depInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr ,
			.imageMemoryBarrierCount = barrierCount,
			.pImageMemoryBarriers = barriers
		};
		vkCmdPipelineBarrier2(commandBuffer, &depInfo);
	}
};

#endif