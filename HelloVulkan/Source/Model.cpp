#include "Model.h"
#include "Configs.h"

#include "assimp/postprocess.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <ranges>

static const std::string BLACK_TEXTURE = "DefaultBlackTexture";

inline glm::mat4 CastToGLMMat4(const aiMatrix4x4& m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

Model::Model(VulkanContext& ctx, const std::string& path) :
	device_(ctx.GetDevice())
{
	// In case a texture type cannot be found, replace it with a black 1x1 texture
	unsigned char black[4] = { 0, 0, 0, 255 };
	AddTexture(ctx, BLACK_TEXTURE, (void*)&black, 1, 1);

	// Load model here
	LoadModel(ctx, path);
}

Model::~Model()
{
	for (Mesh& mesh : meshes_)
	{
		mesh.Destroy();
	}

	for (auto buffer : modelBuffers_)
	{
		buffer.Destroy();
	}

	for (VulkanImage& tex : textureList_)
	{
		tex.Destroy();
	}
}

void Model::AddTexture(VulkanContext& ctx, const std::string& textureFilename)
{
	textureList_.push_back({});
	std::string fullFilePath = this->directory_ + '/' + textureFilename;
	textureList_.back().CreateImageResources(ctx, fullFilePath.c_str());
	textureMap_[textureFilename] = textureList_.size() - 1;
}

void Model::AddTexture(VulkanContext& ctx, const std::string& textureName, void* data, int width, int height)
{
	textureList_.push_back({});
	textureList_.back().CreateImageResources(
		ctx,
		data,
		1,
		1);
	textureMap_[textureName] = textureList_.size() - 1;
}

VulkanImage* Model::GetTexture(int textureIndex)
{
	if (textureIndex < 0 || textureIndex >= textureList_.size())
	{
		std::cerr << "Failed to retrieve a texture because the textureIndex is out of bound\n";
		return nullptr;
	}
	return &(textureList_[textureIndex]);
}

void Model::AddTextureIfEmpty(TextureType tType, const std::string& filePath)
{
	// TODO add to textureMap_

	for (Mesh& mesh : meshes_)
	{
		//mesh.AddTextureIfEmpty(tType, filePath);
	}
}

// Loads a model with supported ASSIMP extensions from file and 
// stores the resulting meshes in the meshes vector.
void Model::LoadModel(VulkanContext& ctx, std::string const& path)
{
	// Read file via ASSIMP
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		path, 
		aiProcess_Triangulate | 
		aiProcess_GenSmoothNormals | 
		aiProcess_FlipUVs | 
		aiProcess_CalcTangentSpace);
	// Check for errors
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		std::cerr << "Error ASSIMP:: " << importer.GetErrorString() << '\n';
		return;
	}

	// Retrieve the directory path of the filepath
	directory_ = path.substr(0, path.find_last_of('/'));

	// Process ASSIMP's root node recursively
	ProcessNode(ctx, scene->mRootNode, scene, glm::mat4(1.0));
}

// Processes a node in a recursive fashion. 
// Processes each individual mesh located at the node and 
// repeats this process on its children nodes (if any).
void Model::ProcessNode(
	VulkanContext& ctx, 
	aiNode* node, 
	const aiScene* scene, 
	const glm::mat4& parentTransform)
{
	glm::mat4 nodeTransform = CastToGLMMat4(node->mTransformation);
	glm::mat4 totalTransform = parentTransform * nodeTransform;

	// Process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		// The node object only contains indices to index the actual objects in the scene. 
		// The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		meshes_.push_back(ProcessMesh(ctx, mesh, scene, totalTransform));
	}
	// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(ctx, node->mChildren[i], scene, totalTransform);
	}
}

Mesh Model::ProcessMesh(
	VulkanContext& ctx, 
	aiMesh* mesh, 
	const aiScene* scene, 
	const glm::mat4& transform)
{
	// Vertices
	std::vector<VertexData> vertices;
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		// Positions
		VertexData vertex =
		{
			.position_ = transform * 
				glm::vec4(
					mesh->mVertices[i].x,
					mesh->mVertices[i].y,
					mesh->mVertices[i].z,
					1)
		};

		// Normals
		if (mesh->HasNormals())
		{
			vertex.normal_ = transform * 
				glm::vec4(
					mesh->mNormals[i].x,
					mesh->mNormals[i].y,
					mesh->mNormals[i].z,
					0
				);
		}

		// UV
		if (mesh->mTextureCoords[0])
		{
			// A vertex can contain up to 8 different texture coordinates. 
			// We thus make the assumption that we won't use models where a vertex 
			// can have multiple texture coordinates so we always take the first set (0).
			vertex.textureCoordinate_ = glm::vec4(
				mesh->mTextureCoords[0][i].x, 
				mesh->mTextureCoords[0][i].y, 
				0, 
				0);
		}
		else
		{
			vertex.textureCoordinate_ = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		}

		vertices.push_back(vertex);
	}

	// Indices
	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		// Retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// PBR textures
	std::unordered_map<TextureType, int> textures;
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	for (auto& aiTType : TextureMapper::aiTTypeSearchOrder)
	{
		auto count = material->GetTextureCount(aiTType);
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
	{
		textures[TextureType::Albedo] = textureMap_[BLACK_TEXTURE];
	}
	if (!textures.contains(TextureType::Normal))
	{
		textures[TextureType::Normal] = textureMap_[BLACK_TEXTURE];
	}
	if (!textures.contains(TextureType::Metalness))
	{
		textures[TextureType::Metalness] = textureMap_[BLACK_TEXTURE];
	}
	if (!textures.contains(TextureType::Roughness))
	{
		textures[TextureType::Roughness] = textureMap_[BLACK_TEXTURE];
	}
	if (!textures.contains(TextureType::AmbientOcclusion))
	{
		textures[TextureType::AmbientOcclusion] = textureMap_[BLACK_TEXTURE];
	}
	if (!textures.contains(TextureType::Emissive))
	{
		textures[TextureType::Emissive] = textureMap_[BLACK_TEXTURE];
	}

	return Mesh(ctx, std::move(vertices), std::move(indices), std::move(textures));
}