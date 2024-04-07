#ifndef VERTEX_DATA
#define VERTEX_DATA

#include "volk.h"
#include "glm/glm.hpp"

#include <vector>

struct VertexData
{

	glm::vec3 position;
	float uvX;
	glm::vec3 normal;
	float uvY;
	glm::vec4 color;
};

namespace VertexUtility
{
	static VkVertexInputBindingDescription GetBindingDescription()
	{
		return
		{
			.binding = 0,
			.stride = sizeof(VertexData),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
	}

	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.push_back(
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(VertexData, position)
		});
		attributeDescriptions.push_back(
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = offsetof(VertexData, uvX)
		});
		attributeDescriptions.push_back(
		{
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(VertexData, normal)
		});
		attributeDescriptions.push_back(
		{
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = offsetof(VertexData, uvY)
		});
		attributeDescriptions.push_back(
		{
			.location = 4,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset = offsetof(VertexData, color)
		});

		return attributeDescriptions;
	}
}

#endif