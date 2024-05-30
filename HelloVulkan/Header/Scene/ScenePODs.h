#ifndef SCENE_PLAIN_OLD_DATA
#define SCENE_PLAIN_OLD_DATA

#include "BoundingBox.h"
#include "VertexData.h"
#include "Configs.h"

#include <vector>
#include <array>

#include "glm/glm.hpp"
#include "glm/ext.hpp"

// Skinning vector with int elements
using iSVec = std::array<int, AppConfig::MaxSkinningBone>;

// Skinning vector with float elements
using fSVec = std::array<float, AppConfig::MaxSkinningBone>;

struct SceneData
{
	std::vector<VertexData> vertices_{};
	std::vector<uint32_t> indices_{};
	std::vector<uint32_t> vertexOffsets_{};
	std::vector<uint32_t> indexOffsets_{};
	
	// Skinning
	uint32_t boneMatrixCount_ = 1u; // Always has at least one matrix (identity)
	std::vector<VertexData> preSkinningVertices_{}; // Subset of vertices_
	std::vector<uint32_t> skinningIndices_{}; // Mapping from preSkinningVertices_ to vertices_
	std::vector<iSVec> boneIDArray_{};
	std::vector<fSVec> boneWeightArray_{};

	uint32_t GetCurrentVertexOffset() const
	{
		if (vertexOffsets_.empty()) { return 0u; }
		return vertexOffsets_.back();
	}

	uint32_t GetCurrentIndexOffset() const
	{
		if (indexOffsets_.empty()) { return 0u; }
		return indexOffsets_.back();
	}
};

enum class MaterialType : uint32_t
{
	Opaque = 0,
	Transparent = 1,
	Specular = 2,
	Light = 3,
};

// For bindless textures
struct MeshData
{
	uint32_t vertexOffset_ = 0;
	uint32_t indexOffset_ = 0;

	// Pointing to modelSSBO_
	uint32_t modelMatrixIndex_ = 0;

	// PBR Texture IDs
	uint32_t albedo_ = 0;
	uint32_t normal_ = 0;
	uint32_t metalness_ = 0;
	uint32_t roughness_ = 0;
	uint32_t ao_ = 0;
	uint32_t emissive_ = 0;

	// For sorting
	MaterialType material_{};
};

struct InstanceData
{
	/*Update model matrix and update the buffer
	Need two indices to access instanceMapArray_*/
	uint32_t modelIndex_ = 0;
	uint32_t perModelInstanceIndex_ = 0;

	// See function Scene::CreateIndirectBuffer()
	uint32_t perModelMeshIndex_ = 0;

	MeshData meshData_{};
	BoundingBox originalBoundingBox_{};
};

// Needed for updating bounding boxes
struct InstanceMap
{
	// Pointing to modelSSBO_
	uint32_t modelMatrixIndex_ = 0;

	// List of global instance indices that share the same model matrix
	std::vector<uint32_t> instanceDataIndices_{};
};

struct ModelCreateInfo
{
	std::string filename{};

	// Allows multiple draw calls 
	uint32_t instanceCount = 1u; 
	
	// No effect if the model does not have animation
	bool playAnimation = false;

	bool clickable = false;
};

// Skinning
struct BoneInfo
{
	// ID is index in finalBoneMatrices
	int id_ = 0; // The first matrix is identity

	// Offset matrix transforms vertex from model space to bone space
	glm::mat4 offsetMatrix_ = glm::mat4(1.0);
};

// Skinning
struct AnimationNode
{
	glm::mat4 transformation_ = glm::mat4(1.0);
	std::string name_{};
	uint32_t childrenCount_ = 0u;
	std::vector<AnimationNode> children_{};
};

// Skinning
struct KeyPosition
{
	glm::vec3 position_{};
	float timeStamp_ = 0.0f;
};

// Skinning
struct KeyRotation
{
	glm::quat orientation_{};
	float timeStamp_ = 0.0f;
};

// Skinning
struct KeyScale
{
	glm::vec3 scale_{};
	float timeStamp_ = 0.0f;
};

#endif