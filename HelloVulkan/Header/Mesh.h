#ifndef MESH
#define MESH

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "TextureMapper.h"
#include "VertexData.h"
#include "UBOs.h"

#include <vector>
#include <unordered_map>

// For bindless textures
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
	bool bindlessTexture_;
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
};

#endif
