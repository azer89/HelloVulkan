#ifndef MESH
#define MESH

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "TextureMapper.h"
#include "UBOs.h"

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

	// Needed to access model matrix
	uint32_t modelIndex;

	// PBR Material
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
	// Slot based
	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;

	std::unordered_map<TextureType, uint32_t> textureIndices_;

private:
	// Bindless rendering
	bool bindless_;
	uint32_t vertexOffset_;
	uint32_t indexOffset_;

	// Slot-based rendering
	std::vector<VertexData> vertices_;
	std::vector<uint32_t> indices_;

	uint32_t numIndices_;

public:
	// TODO Create a default constructor
	Mesh() = default;
	~Mesh() = default;

	void InitSlotBased(
		VulkanContext& ctx,
		uint32_t vertexOffset,
		uint32_t indexOffset,
		std::vector<VertexData>&& _vertices,
		std::vector<uint32_t>&& _indices,
		std::unordered_map<TextureType, uint32_t>&& textureIndices
	);

	void InitBindless(
		VulkanContext& ctx,
		uint32_t vertexOffset,
		uint32_t indexOffset,
		uint32_t numIndices,
		std::unordered_map<TextureType, uint32_t>&& textureIndices);

	uint32_t GetNumIndices() const { return numIndices_; };

	// TODO Implement this function
	//void AddTexture(VulkanContext& ctx, uint32_t textureIndex);

	void SetupSlotBased(VulkanContext& ctx);

	void Destroy();

	MeshData GetMeshData(uint32_t textureIndexOffset, uint32_t modelIndex)
	{
		return
		{
			.vertexOffset_ = vertexOffset_,
			.indexOffset_ = indexOffset_,

			.modelIndex = modelIndex,

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
