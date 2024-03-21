#ifndef MESH
#define MESH

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "TextureMapper.h"
#include "VertexData.h"
#include "UBOs.h"

#include <vector>
#include <unordered_map>

enum class MaterialType : uint32_t
{
	Opaque = 0,
	Transparent = 1,
};

// For bindless textures
struct MeshData
{
	uint32_t vertexOffset_;
	uint32_t indexOffset_;

	// Needed to access model matrix
	uint32_t modelMatrixIndex_;

	// PBR Texture IDs
	uint32_t albedo_;
	uint32_t normal_;
	uint32_t metalness_;
	uint32_t roughness_;
	uint32_t ao_;
	uint32_t emissive_;

	// For sorting
	MaterialType material_;
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
	std::string meshName_;

	uint32_t vertexOffset_;
	uint32_t indexOffset_;

	// Slot-based rendering
	std::vector<VertexData> vertices_;
	std::vector<uint32_t> indices_;

	uint32_t vertexCount_;
	uint32_t indexCount_;

public:
	Mesh() = default;
	~Mesh() = default;

	void InitSlotBased(
		VulkanContext& ctx,
		std::string& meshName,
		uint32_t vertexOffset,
		uint32_t indexOffset,
		std::vector<VertexData>&& _vertices,
		std::vector<uint32_t>&& _indices,
		std::unordered_map<TextureType, uint32_t>&& textureIndices
	);
	void InitBindless(
		VulkanContext& ctx,
		std::string& meshName,
		uint32_t vertexOffset,
		uint32_t indexOffset,
		uint32_t vertexCount,
		uint32_t indexCount,
		std::unordered_map<TextureType, uint32_t>&& textureIndices);
	void SetupSlotBased(VulkanContext& ctx);
	void Destroy();
	// TODO Implement this function
	//void AddTexture(VulkanContext& ctx, uint32_t textureIndex);

	[[nodiscard]] uint32_t GetIndexCount() const { return indexCount_; }
	[[nodiscard]] uint32_t GetVertexOffset() const { return vertexOffset_; }
	[[nodiscard]] uint32_t GetVertexCount() const { return vertexCount_; }

	[[nodiscard]] MeshData GetMeshData(uint32_t textureIndexOffset, uint32_t modelMatrixIndex)
	{
		return
		{
			.vertexOffset_ = vertexOffset_,
			.indexOffset_ = indexOffset_,
			.modelMatrixIndex_ = modelMatrixIndex,
			.albedo_ = textureIndices_[TextureType::Albedo] + textureIndexOffset,
			.normal_ = textureIndices_[TextureType::Normal] + textureIndexOffset,
			.metalness_ = textureIndices_[TextureType::Metalness] + textureIndexOffset,
			.roughness_ = textureIndices_[TextureType::Roughness] + textureIndexOffset,
			.ao_ = textureIndices_[TextureType::AmbientOcclusion] + textureIndexOffset,
			.emissive_ = textureIndices_[TextureType::Emissive] + textureIndexOffset,
			.material_ = GetMaterialType()
		};
	}

	inline MaterialType GetMaterialType()
	{
		if (meshName_.find("transparent") != meshName_.npos)
		{
			return MaterialType::Transparent;
		}
		return MaterialType::Opaque;
	}

	inline void ToLower(std::string& str)
	{
		for (auto& c : str)
		{
			c = tolower(c);
		}
	}
};

#endif
