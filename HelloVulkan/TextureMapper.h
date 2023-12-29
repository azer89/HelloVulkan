#ifndef TEXTURE_TYPE
#define TEXTURE_TYPE

#include "assimp//material.h"

#include <unordered_map>

enum class TextureType : uint32_t
{
	NONE = 0u,
	ALBEDO = 1u,
	NORMAL = 2u,
	METALNESS = 3u,
	ROUGHNESS = 4u,
	AO = 5u,
	EMISSIVE = 6u,
};

// Mapping from ASSIMP textures to PBR textures
namespace TextureMapper
{
	// Corresponds to yhe number of elements in TextureType
	constexpr unsigned int NUM_TEXTURE_TYPE = 6;

	// This vector is a priority list
	static std::vector<aiTextureType> aiTTypeSearchOrder =
	{
		// Diffuse
		aiTextureType_DIFFUSE,

		// Metalness
		aiTextureType_SPECULAR,
		aiTextureType_METALNESS,

		// Normal
		aiTextureType_NORMALS,

		// Roughness
		aiTextureType_DIFFUSE_ROUGHNESS,
		aiTextureType_SHININESS,

		// AO
		aiTextureType_AMBIENT_OCCLUSION,
		aiTextureType_LIGHTMAP,

		// Emissive
		aiTextureType_EMISSIVE
	};

	static std::unordered_map<aiTextureType, TextureType> assimpTextureToTextureType =
	{
		// Diffuse
		{aiTextureType_DIFFUSE, TextureType::ALBEDO},

		// Specular
		{aiTextureType_SPECULAR, TextureType::METALNESS},
		{aiTextureType_METALNESS, TextureType::METALNESS},

		// Normal
		{aiTextureType_NORMALS, TextureType::NORMAL},

		// Roughness shininess
		{aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGHNESS},
		{aiTextureType_SHININESS, TextureType::ROUGHNESS},

		// AO
		{aiTextureType_AMBIENT_OCCLUSION, TextureType::AO},
		{aiTextureType_LIGHTMAP, TextureType::AO},

		// Emissive
		{aiTextureType_EMISSIVE, TextureType::EMISSIVE}
	};

	static TextureType GetTextureType(aiTextureType aiTType)
	{
		if (!TextureMapper::assimpTextureToTextureType.contains(aiTType))
		{
			return TextureType::NONE;
		}
		return TextureMapper::assimpTextureToTextureType[aiTType];
	}
};

#endif
