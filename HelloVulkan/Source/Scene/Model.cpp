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

void Model::LoadSlotBased(VulkanContext& ctx, const std::string& path)
{
	bindlessTexture_ = false;

	// In case a texture type cannot be found, replace it with a black 1x1 texture
	unsigned char black[4] = { 0, 0, 0, 255 };
	AddTexture(ctx, BLACK_TEXTURE, (void*)&black, 1, 1);

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
	const ModelCreateInfo& modelData,
	SceneData& sceneData)
{
	bindlessTexture_ = true;
	modelInfo_ = modelData;

	// In case a texture type cannot be found, replace it with a black 1x1 texture
	unsigned char black[4] = { 0, 0, 0, 255 };
	AddTexture(ctx, BLACK_TEXTURE, (void*)&black, 1, 1);

	// Load model here
	LoadModel(
		ctx,
		modelData.filename,
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

void Model::AddTexture(VulkanContext& ctx, const std::string& textureFilename)
{
	textureList_.emplace_back();
	const std::string fullFilePath = this->directory_ + '/' + textureFilename;
	textureList_.back().CreateImageResources(ctx, fullFilePath.c_str());
	textureMap_[textureFilename] = static_cast<uint32_t>(textureList_.size() - 1);
}

void Model::AddTexture(VulkanContext& ctx, const std::string& textureName, void* data, int width, int height)
{
	textureList_.emplace_back();
	textureList_.back().CreateImageResources(
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

// Loads a model with supported ASSIMP extensions from file and 
// stores the resulting meshes in the meshes vector.
void Model::LoadModel(VulkanContext& ctx,
	std::string const& path,
	SceneData& sceneData)
{
	Assimp::Importer importer;
	scene_ = importer.ReadFile(
		path,
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace);
	// Check for errors
	if (!scene_ || scene_->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene_->mRootNode) // if is Not Zero
	{
		std::cerr << "Error ASSIMP: " << importer.GetErrorString() << '\n';
		return;
	}

	// Retrieve the directory path of the filepath
	directory_ = path.substr(0, path.find_last_of('/'));

	// Process assimp's root node recursively
	ProcessNode(
		ctx,
		sceneData,
		scene_->mRootNode,
		glm::mat4(1.0));
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
	std::vector<VertexData> vertices = GetVertices(mesh, transform);
	std::vector<uint32_t> indices = GetIndices(mesh);
	std::unordered_map<TextureType, uint32_t> textures = GetTextures(ctx, mesh);

	const uint32_t prevVertexOffset = sceneData.GetCurrentVertexOffset();
	const uint32_t prevIndexOffset = sceneData.GetCurrentIndexOffset();

	if (bindlessTexture_)
	{
		sceneData.vertices.insert(std::end(sceneData.vertices), std::begin(vertices), std::end(vertices));
		sceneData.indices.insert(std::end(sceneData.indices), std::begin(indices), std::end(indices));

		// If Bindless textures, we do not move vertices and indices
		meshes_.emplace_back();
		meshes_.back().InitBindless(
			ctx,
			meshName,
			prevVertexOffset,
			prevIndexOffset,
			static_cast<uint32_t>(vertices.size()),
			static_cast<uint32_t>(indices.size()),
			std::move(textures));

		const uint32_t currVertexOffset = static_cast<uint32_t>(vertices.size());
		const uint32_t currIndexOffset = static_cast<uint32_t>(indices.size());

		// Update offsets
		sceneData.vertexOffsets.emplace_back(currVertexOffset + prevVertexOffset);
		sceneData.indexOffsets.emplace_back(currIndexOffset + prevIndexOffset);
	}
	else
	{
		meshes_.emplace_back();
		// If Slot-based we move vertices and indices
		meshes_.back().InitSlotBased(
			ctx,
			meshName,
			prevVertexOffset,
			prevIndexOffset,
			std::move(vertices),
			std::move(indices),
			std::move(textures)
		);
	}
}

std::vector<VertexData> Model::GetVertices(const aiMesh* mesh, const glm::mat4& transform)
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

std::vector<uint32_t> Model::GetIndices(const aiMesh* mesh)
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

std::unordered_map<TextureType, uint32_t> Model::GetTextures(
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

	return textures;
}