#include "Light.h"

void Lights::Destroy()
{
	storageBuffer_.Destroy(device_);
}

void Lights::AddLights(VulkanDevice& vkDev, const std::vector<LightData> lights)
{
	device_ = vkDev.GetDevice();
	storageBufferSize_ = sizeof(LightData) * lights.size();
	AllocateSSBOBuffer(vkDev, lights.data());
}

size_t Lights::AllocateSSBOBuffer(VulkanDevice& vkDev, const void* lightData)
{
	VkDeviceSize bufferSize = storageBufferSize_;

	VulkanBuffer stagingBuffer;
	stagingBuffer.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void* data;
	vkMapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_, 0, bufferSize, 0, &data);
	memcpy(data, lightData, storageBufferSize_);
	vkUnmapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_);

	storageBuffer_.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	storageBuffer_.CopyFrom(vkDev, stagingBuffer.buffer_, bufferSize);

	stagingBuffer.Destroy(vkDev.GetDevice());

	return bufferSize;
}