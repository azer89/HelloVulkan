#ifndef MESH
#define MESH

#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "UBO.h"

#include "volk.h"

#include <vector>
#include <string>

struct VertexData
{
	// TODO add color
	glm::vec4 pos;
	glm::vec4 n;
	glm::vec4 tc;

	static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(VertexData);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexData, pos) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexData, n) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexData, tc) });
		return attributeDescriptions;
	}
};

struct MeshCreateInfo
{
	std::string modelFile;
	std::vector<std::string> textureFiles;
};

class Mesh
{
public:
	size_t vertexBufferSize_;
	size_t indexBufferSize_;

	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;

	std::vector<VulkanTexture> textures_;

	std::vector<VkDescriptorSet> descriptorSets_;

	// ModelUBO
	std::vector<VulkanBuffer> modelBuffers_;

	// Textures
	void AddTexture(VulkanDevice& vkDev, const char* fileName, uint32_t bindIndex);

	// Assimp
	void Create(VulkanDevice& vkDev, const char* filename);

	void Destroy(VkDevice device);

	void SetModelUBO(const VulkanDevice& vkDev, uint32_t imageIndex, ModelUBO ubo)
	{
		UpdateUniformBuffer(vkDev.GetDevice(), modelBuffers_[imageIndex], &ubo, sizeof(ModelUBO));
	}

private:
	// SSBO, currently not used
	VulkanBuffer storageBuffer_;

private:
	void AllocateVertexBuffer(
		VulkanDevice& vkDev,
		const void* vertexData);

	void AllocateIndexBuffer(
		VulkanDevice& vkDev,
		const void* indexData);

	size_t AllocateSSBOBuffer(
		VulkanDevice& vkDev,
		const void* vertexData,
		const void* indexData);

	void UpdateUniformBuffer(
		VkDevice device,
		VulkanBuffer& buffer,
		const void* data,
		const size_t dataSize);
};

#endif
