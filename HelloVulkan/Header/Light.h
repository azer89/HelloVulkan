#ifndef LIGHT
#define LIGHT

#include "glm/glm.hpp"

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

/*
A single light
*/
struct LightData
{
	alignas(16)
	glm::vec4 position_;
	alignas(16)
	glm::vec4 color_ = glm::vec4(1.0f);
	alignas(4)
	float radius_ = 1.0f;
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

	void AddLights(VulkanDevice& vkDev, const std::vector<LightData>& lights);

	VkBuffer GetSSBOBuffer() const { return storageBuffer_.buffer_; }
	size_t GetSSBOSize() const { return storageBufferSize_;  }
	uint32_t GetLightCount() const { return lightCount_; }

public:

private:
	uint32_t lightCount_;

	size_t storageBufferSize_;
	VulkanBuffer storageBuffer_;

	VkDevice device_;
};

#endif