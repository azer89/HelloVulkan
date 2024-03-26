#ifndef SCENE_PLAIN_OLD_DATA
#define SCENE_PLAIN_OLD_DATA

#include "VertexData.h"

#include <vector>

struct SceneData
{
	std::vector<VertexData> vertices = {};
	std::vector<uint32_t> indices = {};
	std::vector<uint32_t> vertexOffsets = {};
	std::vector<uint32_t> indexOffsets = {};

	uint32_t GetCurrentVertexOffset()
	{
		if (vertexOffsets.empty())
		{
			return 0u;
		}
		return vertexOffsets.back();
	}

	uint32_t GetCurrentIndexOffset()
	{
		if (indexOffsets.empty())
		{
			return 0u;
		}
		return indexOffsets.back();
	}
};

struct InstanceData
{
	uint32_t modelIndex;
	uint32_t meshIndex;
	uint32_t perModelInstanceIndex;
	MeshData meshData;
	BoundingBox originalBoundingBox;
};

// Needed for updating bounding boxes
struct InstanceMap
{
	uint32_t modelMatrixIndex;
	std::vector<uint32_t> instanceDataIndices;
};

struct ModelCreateInfo
{
	std::string filename;
	uint32_t instanceCount; // Allows instancing
};

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

#endif