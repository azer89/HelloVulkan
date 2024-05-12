#include "Model.h"
#include "Configs.h"

#include "assimp/postprocess.h"
#include "assimp/Importer.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <ranges>

static const std::string DEFAULT_BLACK_TEXTURE = "DefaultBlackTexture";
static const std::string DEFAULT_NORMAL_TEXTURE = "DefaultNormalTexture";

inline glm::mat4 CastToGLMMat4(const aiMatrix4x4& m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

void Model::LoadSlotBased(VulkanContext& ctx, const std::string& path)
{
	bindlessTexture_ = false;

	CreateDefaultTextures(ctx);

	// Load model here
	SceneData dummySceneData = {};
	LoadModel(
		ctx,
		path,
		dummySceneData);

	// Slot-based rendering
	CreateModelUBOBuffers(ctx);
}

void Model::LoadBindless(
	VulkanContext& ctx,
	const ModelCreateInfo& modelInfo,
	SceneData& sceneData)
{
	bindlessTexture_ = true;
	modelInfo_ = modelInfo;

	CreateDefaultTextures(ctx);

	// Load model here
	LoadModel(
		ctx,
		modelInfo_.filename,
		sceneData);
}

void Model::Destroy()
{
	for (Mesh& mesh : meshes_)
	{
		mesh.Destroy();
	}
	for (auto& buffer : modelBuffers_)
	{
		buffer.Destroy();
	}
	for (VulkanImage& tex : textureList_)
	{
		tex.Destroy();
	}
}

void Model::CreateModelUBOBuffers(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;
	modelBuffers_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		modelBuffers_[i].CreateBuffer(
			ctx,
			sizeof(ModelUBO),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);
	}
}

void Model::SetModelUBO(VulkanContext& ctx, ModelUBO ubo)
{
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		modelBuffers_[i].UploadBufferData(ctx, &ubo, sizeof(ModelUBO));
	}
}

void Model::CreateDefaultTextures(VulkanContext& ctx)
{
	uint32_t black = 0xff000000;
	AddTexture(ctx, DEFAULT_BLACK_TEXTURE, (void*)&black, 1, 1);

	// TODO Investigate a correct value for normal vector
	uint32_t normal = 0xffff8888;
	AddTexture(ctx, DEFAULT_NORMAL_TEXTURE, (void*)&normal, 1, 1);
}

// Loads a model with supported ASSIMP extensions from file and 
// stores the resulting meshes in the meshes vector.
void Model::LoadModel(VulkanContext& ctx,
	std::string const& path,
	SceneData& sceneData)
{
	filepath_ = path;
	Assimp::Importer importer;
	scene_ = importer.ReadFile(
		filepath_,
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
	// Check for errors
	if (!scene_ || scene_->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene_->mRootNode) // if is Not Zero
	{
		throw std::runtime_error("Cannot load file " + path);
	}

	// Retrieve the directory path of the filepath
	directory_ = filepath_.substr(0, filepath_.find_last_of('/'));

	processAnimation_ = modelInfo_.playAnimation && scene_->mAnimations;
	if (processAnimation_) { boneCounter_ = static_cast<int>(sceneData.boneMatrixCount_); }

	// Process assimp's root node recursively
	ProcessNode(
		ctx,
		sceneData,
		scene_->mRootNode,
		glm::mat4(1.0));

	if (processAnimation_) { sceneData.boneMatrixCount_ += AppConfig::MaxSkinningMatrices; }
}

// Processes a node in a recursive fashion.
void Model::ProcessNode(
	VulkanContext& ctx,
	SceneData& sceneData,
	const aiNode* node,
	const glm::mat4& parentTransform)
{
	const glm::mat4 nodeTransform = CastToGLMMat4(node->mTransformation);
	const glm::mat4 totalTransform = parentTransform * nodeTransform;

	// Process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		const aiMesh* mesh = scene_->mMeshes[node->mMeshes[i]];
		ProcessMesh(
			ctx,
			sceneData,
			mesh,
			totalTransform);
	}
	// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(
			ctx,
			sceneData,
			node->mChildren[i],
			totalTransform);
	}
}

void Model::ProcessMesh(
	VulkanContext& ctx,
	SceneData& sceneData,
	const aiMesh* mesh,
	const glm::mat4& transform)
{
	const std::string meshName = mesh->mName.C_Str();

	std::vector<VertexData> vertices = GetMeshVertices(mesh, processAnimation_ ? glm::mat4(1.0f) : transform);
	std::vector<uint32_t> indices = GetMeshIndices(mesh);
	std::unordered_map<TextureType, uint32_t> textureIndices = GetTextureIndices(ctx, mesh);

	const uint32_t prevVertexOffset = sceneData.GetCurrentVertexOffset();
	const uint32_t prevIndexOffset = sceneData.GetCurrentIndexOffset();

	std::vector<uint32_t> skinningIndices;
	std::vector<iSVec> boneIDArray;
	std::vector<fSVec> boneWeightArray;
	if (processAnimation_)
	{
		SetBoneToDefault(
			skinningIndices, 
			boneIDArray, 
			boneWeightArray, 
			static_cast<uint32_t>(vertices.size()),
			prevVertexOffset);
		ExtractBoneWeight(boneIDArray, boneWeightArray, mesh);
	}

	if (bindlessTexture_)
	{
		sceneData.vertices_.insert(std::end(sceneData.vertices_), std::begin(vertices), std::end(vertices));
		sceneData.indices_.insert(std::end(sceneData.indices_), std::begin(indices), std::end(indices));

		if (processAnimation_)
		{
			sceneData.boneIDArray_.insert(std::end(sceneData.boneIDArray_), std::begin(boneIDArray), std::end(boneIDArray));
			sceneData.boneWeightArray_.insert(std::end(sceneData.boneWeightArray_), std::begin(boneWeightArray), std::end(boneWeightArray));
			sceneData.preSkinningVertices_.insert(std::end(sceneData.preSkinningVertices_), std::begin(vertices), std::end(vertices));
			sceneData.skinningIndices_.insert(std::end(sceneData.skinningIndices_), std::begin(skinningIndices), std::end(skinningIndices));
		}

		// If Bindless textures, we do not move vertices and indices
		meshes_.emplace_back().InitBindless(
			ctx,
			meshName,
			prevVertexOffset,
			prevIndexOffset,
			static_cast<uint32_t>(vertices.size()),
			static_cast<uint32_t>(indices.size()),
			std::move(textureIndices));

		const uint32_t currVertexOffset = static_cast<uint32_t>(vertices.size());
		const uint32_t currIndexOffset = static_cast<uint32_t>(indices.size());

		// Update offsets
		sceneData.vertexOffsets_.emplace_back(currVertexOffset + prevVertexOffset);
		sceneData.indexOffsets_.emplace_back(currIndexOffset + prevIndexOffset);
	}
	else
	{
		// If Slot-based we move vertices and indices
		meshes_.emplace_back().InitSlotBased(
			ctx,
			meshName,
			prevVertexOffset,
			prevIndexOffset,
			std::move(vertices),
			std::move(indices),
			std::move(textureIndices)
		);
	}
}

void Model::SetBoneToDefault(
	std::vector<uint32_t>& skinningIndices,
	std::vector<iSVec>& boneIDArray,
	std::vector<fSVec>& boneWeightArray,
	uint32_t vertexCount,
	uint32_t prevVertexOffset)
{
	skinningIndices.resize(vertexCount);
	boneIDArray.resize(vertexCount);
	boneWeightArray.resize(vertexCount);

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		skinningIndices[i] = prevVertexOffset + i;
		for (uint32_t j = 0; j < AppConfig::MaxSkinningBone; ++j)
		{
			boneIDArray[i][j] = 0; // The first matrix is identity
			boneWeightArray[i][j] = 0.0f;
		}
	}
}

void Model::ExtractBoneWeight(
	std::vector<iSVec>& boneIDs,
	std::vector<fSVec>& boneWeights,
	const aiMesh* mesh)
{
	if (mesh->mNumBones == 0)
	{
		return;
	}

	for (uint32_t b = 0; b < mesh->mNumBones; ++b)
	{
		int boneID = -1;
		std::string boneName = mesh->mBones[b]->mName.C_Str();
		if (!boneInfoMap_.contains(boneName))
		{
			boneInfoMap_[boneName] =
			{
				.id_ = boneCounter_,
				.offsetMatrix_ = CastToGLMMat4(mesh->mBones[b]->mOffsetMatrix)
			};
			boneID = boneCounter_;
			++boneCounter_;
		}
		else
		{
			boneID = boneInfoMap_[boneName].id_;
		}

		if (boneID < 0)
		{
			std::cerr << "Cannot find bone, name = " << boneName << '\n';
		}

		const aiVertexWeight* weights = mesh->mBones[b]->mWeights;
		const uint32_t numWeights = mesh->mBones[b]->mNumWeights;

		for (uint32_t w = 0; w < numWeights; ++w)
		{
			const uint32_t vertexId = weights[w].mVertexId;
			const float weight = weights[w].mWeight;
			assert(vertexId < mesh->mNumVertices);

			// NOTE Do not consider a bone if the weight is zero
			if (weight == 0)
			{
				continue;
			}

			for (uint32_t iter = 0; iter < AppConfig::MaxSkinningBone; ++iter)
			{
				if (boneIDs[vertexId][iter] == 0)
				{
					boneWeights[vertexId][iter] = weight;
					boneIDs[vertexId][iter] = boneID;
					break;
				}
			}
		}
	}
}

std::vector<VertexData> Model::GetMeshVertices(const aiMesh* mesh, const glm::mat4& transform)
{
	std::vector<VertexData> vertices;
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		// Positions
		VertexData vertex =
		{
			.position = glm::vec3(transform *
				glm::vec4(
					mesh->mVertices[i].x,
					mesh->mVertices[i].y,
					mesh->mVertices[i].z,
					1))
		};

		// Normals
		if (mesh->HasNormals())
		{
			vertex.normal = transform *
				glm::vec4(
					mesh->mNormals[i].x,
					mesh->mNormals[i].y,
					mesh->mNormals[i].z,
					0
				);
		}

		// UV
		vertex.uvX = 0.f;
		vertex.uvY = 0.f;
		if (mesh->mTextureCoords[0])
		{
			// A vertex can contain up to 8 different texture coordinates. 
			// We thus make the assumption that we won't use models where a vertex 
			// can have multiple texture coordinates so we always take the first set (0).
			vertex.uvX = mesh->mTextureCoords[0][i].x;
			vertex.uvY = mesh->mTextureCoords[0][i].y;
		}

		// Color
		vertex.color = glm::vec4(0.f);
		if (mesh->mColors[0])
		{
			auto color = mesh->mColors[0][i];
			vertex.color = glm::vec4(color.r, color.g, color.b, color.a);
		}

		vertices.push_back(vertex);
	}
	return vertices;
}

std::vector<uint32_t> Model::GetMeshIndices(const aiMesh* mesh)
{
	std::vector<uint32_t> indices;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		const aiFace& face = mesh->mFaces[i];
		// Retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(static_cast<uint32_t>(face.mIndices[j]));
		}
	}
	return indices;
}

void Model::AddTexture(VulkanContext& ctx, const std::string& textureFilename)
{
	const std::string fullFilePath = this->directory_ + '/' + textureFilename;
	textureList_.emplace_back().CreateImageResources(ctx, fullFilePath.c_str());
	textureMap_[textureFilename] = static_cast<uint32_t>(textureList_.size() - 1);
}

void Model::AddTexture(VulkanContext& ctx, const std::string& textureName, void* data, int width, int height)
{
	textureList_.emplace_back().CreateImageResources(
		ctx,
		data,
		1,
		1);
	textureMap_[textureName] = static_cast<uint32_t>(textureList_.size() - 1);
}

VulkanImage* Model::GetTexture(uint32_t textureIndex)
{
	if (textureIndex < 0 || textureIndex >= textureList_.size())
	{
		std::cerr << "Failed to retrieve a texture because the textureIndex is out of bound\n";
		return nullptr;
	}
	return &(textureList_[textureIndex]);
}

std::unordered_map<TextureType, uint32_t> Model::GetTextureIndices(
	VulkanContext& ctx,
	const aiMesh* mesh)
{
	// PBR textures
	std::unordered_map<TextureType, uint32_t> textures;
	const aiMaterial* material = scene_->mMaterials[mesh->mMaterialIndex];
	for (const auto& aiTType : TextureMapper::aiTTypeSearchOrder)
	{
		const auto count = material->GetTextureCount(aiTType);
		for (unsigned int i = 0; i < count; ++i)
		{
			aiString str;
			material->GetTexture(aiTType, i, &str);
			std::string filename = str.C_Str();
			TextureType tType = TextureMapper::GetTextureType(aiTType);

			// Make sure each texture is loaded once
			if (!textureMap_.contains(filename))
			{
				AddTexture(ctx, filename);
			}

			// Only support one image per texture type, if we happen to load 
			// multiple textures of the same type, we only use one.
			if (!textures.contains(tType))
			{
				textures[tType] = textureMap_[filename];
			}
		}
	}

	// Replace missing PBR textures with a black 1x1 texture
	if (!textures.contains(TextureType::Albedo))
		{ textures[TextureType::Albedo] = textureMap_[DEFAULT_BLACK_TEXTURE]; }
	if (!textures.contains(TextureType::Normal))
		{ textures[TextureType::Normal] = textureMap_[DEFAULT_NORMAL_TEXTURE]; }
	if (!textures.contains(TextureType::Metalness))
		{ textures[TextureType::Metalness] = textureMap_[DEFAULT_BLACK_TEXTURE]; }
	if (!textures.contains(TextureType::Roughness))
		{ textures[TextureType::Roughness] = textureMap_[DEFAULT_BLACK_TEXTURE]; }
	if (!textures.contains(TextureType::AmbientOcclusion))
		{ textures[TextureType::AmbientOcclusion] = textureMap_[DEFAULT_BLACK_TEXTURE]; }
	if (!textures.contains(TextureType::Emissive))
		{ textures[TextureType::Emissive] = textureMap_[DEFAULT_BLACK_TEXTURE]; }

	return textures;
}