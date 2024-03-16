#include "ResourcesLight.h"

void ResourcesLight::Destroy()
{
	storageBuffer_.Destroy();
}

void ResourcesLight::AddLights(VulkanContext& ctx, const std::vector<LightData>& lights)
{
	device_ = ctx.GetDevice();
	lights_ = lights;
	const VkDeviceSize storageBufferSize = sizeof(LightData) * lights.size();
	lightCount_ = static_cast<uint32_t>(lights.size());
	storageBuffer_.CreateBuffer(ctx, storageBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	storageBuffer_.UploadBufferData(ctx, lights.data(), storageBufferSize);
}

void ResourcesLight::UpdateLightPosition(VulkanContext& ctx, size_t index, float* position)
{
	if (index < 0 || index >= lights_.size())
	{
		return;
	}
	// TODO Do not update if positions approximately the same
	lights_[index].position_ = {position[0], position[1], position[2], 1.0};
	storageBuffer_.UploadOffsetBufferData(ctx, &(lights_[index]), sizeof(LightData) * index, sizeof(LightData));
}