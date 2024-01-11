#ifndef LIGHT
#define LIGHT

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

struct LightData
{
	glm::vec4 position_;
	glm::vec4 color_;
};

class Lights
{
public:
	Lights() = default;
	~Lights() = default;

	void Destroy();

	void AddLights(VulkanDevice& vkDev, const std::vector<LightData> lights);

	VkBuffer GetSSBOBuffer() { return storageBuffer_.buffer_; }
	size_t GetSSBOSize() { return storageBufferSize_;  }

public:
	size_t AllocateSSBOBuffer(
		VulkanDevice& vkDev,
		const void* lightData);

private:
	size_t storageBufferSize_;
	VulkanBuffer storageBuffer_;

	VkDevice device_;
};

#endif