#ifndef MESH
#define MESH

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "TextureMapper.h"
#include "VertexData.h"
#include "ScenePODs.h"

#include <vector>
#include <unordered_map>

class Mesh
{
public:
	// Slot based
	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;

	std::unordered_map<TextureType, uint32_t> textureIndices_;

private:
	bool bindlessTexture_ = false;
	std::string meshName_ = {};

	uint32_t vertexOffset_ = 0;
	uint32_t indexOffset_ = 0;

	// Slot-based rendering
	std::vector<VertexData> vertices_ = {};
	std::vector<uint32_t> indices_ = {};

	uint32_t vertexCount_ = 0;
	uint32_t indexCount_ = 0;

public:
	Mesh() = default;
	~Mesh() = default;

	void InitSlotBased(
		VulkanContext& ctx,
		const std::string& meshName,
		const uint32_t vertexOffset,
		const uint32_t indexOffset,
		std::vector<VertexData>&& _vertices,
		std::vector<uint32_t>&& _indices,
		std::unordered_map<TextureType, uint32_t>&& textureIndices
	);
	void InitBindless(
		VulkanContext& ctx,
		const std::string& meshName,
		const uint32_t vertexOffset,
		const uint32_t indexOffset,
		const uint32_t vertexCount,
		const uint32_t indexCount,
		std::unordered_map<TextureType, uint32_t>&& textureIndices);
	void SetupSlotBased(VulkanContext& ctx);
	void Destroy();

	[[nodiscard]] uint32_t GetIndexOffset() const { return indexOffset_; }
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

	[[nodiscard]] MaterialType GetMaterialType() const
	{
		if (meshName_.find("transparent") != meshName_.npos)
		{
			return MaterialType::Transparent;
		}
		return MaterialType::Opaque;
	}

	void ToLower(std::string& str) const
	{
		for (auto& c : str)
		{
			// TODO Narrowing conversion
			c = tolower(c);
		}
	}
};

#endif
