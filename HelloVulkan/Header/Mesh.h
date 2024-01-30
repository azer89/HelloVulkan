#ifndef MESH
#define MESH

#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "TextureMapper.h"
#include "UBO.h"

#include "volk.h"

#include <vector>
#include <unordered_map>

struct VertexData
{
	// TODO add color
	glm::vec4 position_;
	glm::vec4 normal_;
	glm::vec4 textureCoordinate_;

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
		attributeDescriptions.push_back(
		{ 
			.location = 0, 
			.binding = 0, 
			.format = VK_FORMAT_R32G32B32A32_SFLOAT, 
			.offset = offsetof(VertexData, position_) 
		});
		attributeDescriptions.push_back(
		{ 
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset = offsetof(VertexData, normal_)
		});
		attributeDescriptions.push_back(
		{ 
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset = offsetof(VertexData, textureCoordinate_)
		});
		return attributeDescriptions;
	}
};

class Mesh
{
public:
	size_t vertexBufferSize_;
	size_t indexBufferSize_;

	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;

	std::vector<VertexData> vertices_;
	std::vector<unsigned int> indices_;
	std::unordered_map<TextureType, VulkanImage*> textures_;

	std::vector<VkDescriptorSet> descriptorSets_;

	// Constructors
	Mesh(
		VulkanDevice& vkDev,
		std::vector<VertexData>&& _vertices,
		std::vector<unsigned int>&& _indices,
		std::unordered_map<TextureType, VulkanImage*>&& _textures);
	Mesh(
		VulkanDevice& vkDev,
		const std::vector<VertexData>& vertices,
		const std::vector<unsigned int>& indices,
		const std::unordered_map<TextureType, VulkanImage*>& textures);

	// Textures
	//void AddTexture(VulkanDevice& vkDev, const char* fileName, uint32_t bindIndex);

	void Setup(VulkanDevice& vkDev);

	// Assimp
	void Create(VulkanDevice& vkDev, const char* filename);

	void Destroy(VkDevice device);

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

	size_t AllocateBindlessBuffer(
		VulkanDevice& vkDev,
		const void* vertexData,
		const void* indexData);
};

#endif
