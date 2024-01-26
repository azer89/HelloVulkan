#include "Light.h"

void Lights::Destroy()
{
	storageBuffer_.Destroy(device_);
}

void Lights::AddLights(VulkanDevice& vkDev, const std::vector<LightData>& lights)
{
	device_ = vkDev.GetDevice();
	storageBufferSize_ = sizeof(LightData) * lights.size();
	lightCount_ = static_cast<uint32_t>(lights.size());
	storageBuffer_.CreateSharedBuffer(vkDev, storageBufferSize_,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	storageBuffer_.UploadBufferData(vkDev, 0, lights.data(), storageBufferSize_);
}