#ifndef SCENE_PLAIN_OLD_DATA
#define SCENE_PLAIN_OLD_DATA

#include "VertexData.h"
#include "BoundingBox.h"
#include "Configs.h"

#include <vector>
#include <array>

#include "glm/glm.hpp"
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

// Skinning vector with uint elements
using uSVec = std::array<uint32_t, AppConfig::MaxSkinningBone>;

// Skinning vector with float elements
using fSVec = std::array<float, AppConfig::MaxSkinningBone>;

struct SceneData
{
	std::vector<VertexData> vertices = {};
	std::vector<uint32_t> indices = {};
	std::vector<uint32_t> vertexOffsets = {};
	std::vector<uint32_t> indexOffsets = {};
	
	// Skinning
	std::vector<uSVec> boneIDs;
	std::vector<fSVec> boneWeights;

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

// Skinning
struct BoneInfo
{
	// ID is index in finalBoneMatrices
	int id = -1;

	// Offset matrix transforms vertex from model space to bone space
	glm::mat4 offsetMatrix = glm::mat4(1.0);

};

// Skinning
struct AnimationNode
{
	glm::mat4 transformation = glm::mat4(1.0);
	std::string name = {};
	uint32_t childrenCount = 0u;
	std::vector<AnimationNode> children = {};
};

// Skinning
struct KeyPosition
{
	glm::vec3 position;
	float timeStamp;
};

// Skinning
struct KeyRotation
{
	glm::quat orientation;
	float timeStamp;
};

// Skinning
struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};

#endif