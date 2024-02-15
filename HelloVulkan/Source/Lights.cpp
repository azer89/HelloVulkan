#include "Light.h"

void Lights::Destroy()
{
	storageBuffer_.Destroy();
}

void Lights::AddLights(VulkanContext& vkDev, const std::vector<LightData>& lights)
{
	device_ = vkDev.GetDevice();
	storageBufferSize_ = sizeof(LightData) * lights.size();
	lightCount_ = static_cast<uint32_t>(lights.size());
	storageBuffer_.CreateBuffer(vkDev, storageBufferSize_,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	storageBuffer_.UploadBufferData(vkDev, 0, lights.data(), storageBufferSize_);
}