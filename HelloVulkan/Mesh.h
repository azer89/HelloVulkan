#ifndef MESH
#define MESH

#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

#define VK_NO_PROTOTYPES
#include "volk.h"

#include <vector>

class Mesh
{
public:
	size_t vertexBufferSize_;
	size_t indexBufferSize_;

	VulkanBuffer storageBuffer_;

	std::vector<VulkanTexture> textures_;

	// Textures
	void AddTexture(VulkanDevice& vkDev, const char* fileName, uint32_t bindIndex);

	// Assimp
	void Create(VulkanDevice& vkDev, const char* filename);

	void Destroy(VkDevice device);

private:
	size_t AllocateVertexBuffer(
		VulkanDevice& vkDev,
		const void* vertexData,
		const void* indexData);
};

#endif
