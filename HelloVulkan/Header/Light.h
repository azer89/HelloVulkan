#ifndef LIGHT
#define LIGHT

#include "glm/glm.hpp"

#include "VulkanContext.h"
#include "VulkanBuffer.h"

// A single light
struct LightData
{
	alignas(16)
	glm::vec4 position_;
	alignas(16)
	glm::vec4 color_ = glm::vec4(1.0f);
	alignas(4)
	float radius_ = 1.0f;
};

// Clustered forward
struct AABB
{
	alignas(16)
	glm::vec4 minPoint;
	alignas(16)
	glm::vec4 maxPoint;
};

// Clustered forward
struct LightCell
{
	alignas(4)
	uint32_t offset;
	alignas(4)
	uint32_t count;
};

// A collection of lights, including SSBO
class Lights
{
public:
	Lights() = default;
	~Lights() = default;

	void Destroy();

	void AddLights(VulkanContext& ctx, const std::vector<LightData>& lights);

	void UpdateLightPosition(VulkanContext& ctx, size_t index, float* position);

	VulkanBuffer* GetVulkanBufferPtr() { return &storageBuffer_;  }
	uint32_t GetLightCount() const { return lightCount_; }
	VkDescriptorBufferInfo GetBufferInfo() const
	{
		return
		{
			.buffer = storageBuffer_.buffer_,
			.offset = 0,
			.range = storageBuffer_.size_
		};
	}

public:
	std::vector<LightData> lights_;

private:
	uint32_t lightCount_;
	VulkanBuffer storageBuffer_;
	VkDevice device_;
};

#endif