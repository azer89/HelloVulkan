#ifndef LIGHT
#define LIGHT

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

/*
A single light
*/
struct LightData
{
	glm::vec4 position_;
	glm::vec4 color_;
};

/*
A collection of lights, including SSBO
*/
class Lights
{
public:
	Lights() = default;
	~Lights() = default;

	void Destroy();

	void AddLights(VulkanDevice& vkDev, const std::vector<LightData> lights);

	VkBuffer GetSSBOBuffer() const { return storageBuffer_.buffer_; }
	size_t GetSSBOSize() const { return storageBufferSize_;  }
	uint32_t GetLightCount() const { return lightCount_; }

public:
	// TODO Move this to VulkanBuffer
	size_t AllocateSSBOBuffer(
		VulkanDevice& vkDev,
		const void* lightData);

private:
	uint32_t lightCount_;

	size_t storageBufferSize_;
	VulkanBuffer storageBuffer_;

	VkDevice device_;
};

#endif