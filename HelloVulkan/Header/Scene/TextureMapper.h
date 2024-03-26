#ifndef TEXTURE_MAPPER
#define TEXTURE_MAPPER

#include "assimp//material.h"

#include <unordered_map>

enum class TextureType : uint8_t
{
	None = 0u,
	Albedo = 1u,
	Normal = 2u,
	Metalness = 3u,
	Roughness = 4u,
	AmbientOcclusion = 5u,
	Emissive = 6u,
};

// Mapping from ASSIMP textures to PBR textures
namespace TextureMapper
{
	// Corresponds to yhe number of elements in TextureType
	constexpr unsigned int NUM_TEXTURE_TYPE = 6;

	// This vector is a priority list
	static std::vector<aiTextureType> aiTTypeSearchOrder =
	{
		// Albedo
		aiTextureType_DIFFUSE,

		// Metalness
		aiTextureType_SPECULAR,
		aiTextureType_METALNESS,

		// Normal
		aiTextureType_NORMALS,

		// Roughness
		aiTextureType_DIFFUSE_ROUGHNESS,
		aiTextureType_SHININESS,

		// AmbientOcclusion
		aiTextureType_AMBIENT_OCCLUSION,
		aiTextureType_LIGHTMAP,

		// Emissive
		aiTextureType_EMISSIVE
	};

	static std::unordered_map<aiTextureType, TextureType> assimpTextureToTextureType =
	{
		// Albedo
		{aiTextureType_DIFFUSE, TextureType::Albedo},

		// Specular
		{aiTextureType_SPECULAR, TextureType::Metalness},
		{aiTextureType_METALNESS, TextureType::Metalness},

		// Normal
		{aiTextureType_NORMALS, TextureType::Normal},

		// Roughness shininess
		{aiTextureType_DIFFUSE_ROUGHNESS, TextureType::Roughness},
		{aiTextureType_SHININESS, TextureType::Roughness},

		// Ambient Occlusion
		{aiTextureType_AMBIENT_OCCLUSION, TextureType::AmbientOcclusion},
		{aiTextureType_LIGHTMAP, TextureType::AmbientOcclusion},

		// Emissive
		{aiTextureType_EMISSIVE, TextureType::Emissive}
	};

	static TextureType GetTextureType(aiTextureType aiTType)
	{
		if (!TextureMapper::assimpTextureToTextureType.contains(aiTType))
		{
			return TextureType::None;
		}
		return TextureMapper::assimpTextureToTextureType[aiTType];
	}
};

#endif
