#ifndef MESH
#define MESH

#include "VulkanContext.h"
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
	// TODO use alignas
	glm::vec4 position_;
	glm::vec4 normal_;
	glm::vec4 textureCoordinate_;
};

// For bindless
struct MeshData
{
	uint32_t vertexOffset_;
	uint32_t indexOffset_;

	uint32_t albedo_;
	uint32_t normal_;
	uint32_t metalness_;
	uint32_t roughness_;
	uint32_t ao_;
	uint32_t emissive_;
};

class Mesh
{
public:
	// Bindless rendering
	uint32_t vertexOffset_;
	uint32_t indexOffset_;

	// Bind-ful rendering
	VkDeviceSize vertexBufferSize_;
	VkDeviceSize indexBufferSize_;
	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;

	bool bindless_;
	std::vector<VertexData> vertices_;
	std::vector<uint32_t> indices_;
	std::unordered_map<TextureType, uint32_t> textureIndices_;

	// Constructors
	Mesh(
		VulkanContext& ctx,
		bool bindless,
		uint32_t vertexOffset,
		uint32_t indexOffset,
		std::vector<VertexData>&& _vertices,
		std::vector<uint32_t>&& _indices,
		std::unordered_map<TextureType, uint32_t>&& textureIndices);
	Mesh(
		VulkanContext& ctx,
		bool bindless,
		uint32_t vertexOffset,
		uint32_t indexOffset,
		const std::vector<VertexData>& vertices,
		const std::vector<uint32_t>& indices,
		const std::unordered_map<TextureType, uint32_t>& textureIndices);

	// TODO Implement this function
	//void AddTexture(VulkanContext& ctx, const char* fileName);

	void Setup(VulkanContext& ctx);

	void Destroy();

	MeshData GetMeshData(uint32_t textureIndexOffset)
	{
		return
		{
			.vertexOffset_ = vertexOffset_,
			.indexOffset_ = indexOffset_,

			.albedo_ = textureIndices_[TextureType::Albedo] + textureIndexOffset,
			.normal_ = textureIndices_[TextureType::Normal] + textureIndexOffset,
			.metalness_ = textureIndices_[TextureType::Metalness] + textureIndexOffset,
			.roughness_ = textureIndices_[TextureType::Roughness] + textureIndexOffset,
			.ao_ = textureIndices_[TextureType::AmbientOcclusion] + textureIndexOffset,
			.emissive_ = textureIndices_[TextureType::Emissive] + textureIndexOffset,
		};
	}

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

#endif
