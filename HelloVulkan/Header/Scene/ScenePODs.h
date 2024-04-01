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
	std::vector<VertexData> vertices = {};
	std::vector<uint32_t> indices = {};
	std::vector<uint32_t> vertexOffsets = {};
	std::vector<uint32_t> indexOffsets = {};
	
	// Skinning
	uint32_t boneMatrixCount = 1u; // Always has at least one matrix (identity)
	std::vector<VertexData> preSkinningVertices = {}; // Subset of vertices
	std::vector<uint32_t> skinningIndices = {}; // Mapping from preSkinningVertices to vertices
	std::vector<iSVec> boneIDArray = {};
	std::vector<fSVec> boneWeightArray = {};

	uint32_t GetCurrentVertexOffset() const
	{
		if (vertexOffsets.empty()) { return 0u; }
		return vertexOffsets.back();
	}

	uint32_t GetCurrentIndexOffset() const
	{
		if (indexOffsets.empty()) { return 0u; }
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
	MaterialType material_ = {};
};

struct InstanceData
{
	/*Update model matrix and update the buffer
	Need two indices to access instanceMapArray_
		First index is modelIndex
		Second index is perModelInstanceIndex*/
	uint32_t modelIndex = 0;
	uint32_t perModelInstanceIndex = 0;

	// See function Scene::CreateIndirectBuffer()
	uint32_t perModelMeshIndex = 0;

	MeshData meshData = {};
	BoundingBox originalBoundingBox = {};
};

// Needed for updating bounding boxes
struct InstanceMap
{
	// Pointing to modelSSBO_
	uint32_t modelMatrixIndex = 0;

	// List of global instance indices that share the same model matrix
	std::vector<uint32_t> instanceDataIndices = {};
};

struct ModelCreateInfo
{
	std::string filename = {};

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
	int id = 0; // The first matrix is identity

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
	glm::vec3 position = {};
	float timeStamp = 0.0f;
};

// Skinning
struct KeyRotation
{
	glm::quat orientation = {};
	float timeStamp = 0.0f;
};

// Skinning
struct KeyScale
{
	glm::vec3 scale = {};
	float timeStamp = 0.0f;
};

#endif