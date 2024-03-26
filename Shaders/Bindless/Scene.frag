#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

/*
Bindless/Scene.frag 

Fragment shader for 
	* Bindless textures
	* PBR+IBL
	* Naive forward shading (non clustered)
*/

// Include files
#include <CameraUBO.glsl>
#include <LightData.glsl>
#include <PBRHeader.glsl>
#include <PBRPushConstants.glsl>
#include <Hammersley.glsl>
#include <TangentNormalToWorld.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/BDA.glsl>

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 vertexColor;
layout(location = 4) in flat uint meshIndex;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PC { PBRPushConstant pc; };

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; }; // UBO
layout(set = 0, binding = 2) uniform BDABlock { BDA bda; }; // UBO
layout(set = 0, binding = 3) readonly buffer Lights { LightData lights []; };// SSBO

layout(set = 0, binding = 4) uniform samplerCube specularMap;
layout(set = 0, binding = 5) uniform samplerCube diffuseMap;
layout(set = 0, binding = 6) uniform sampler2D brdfLUT;

// NOTE This requires descriptor indexing feature
layout(set = 0, binding = 7) uniform sampler2D pbrTextures[];

#include <Radiance.glsl>
#include <Ambient.glsl>

float LinearDepth(float z, float near, float far)
{
	return near * far / (far + z * (near - far));
}

void main()
{
	MeshData mData = bda.meshReference.meshes[meshIndex];

	vec4 albedo4 = texture(pbrTextures[nonuniformEXT(mData.albedo)], texCoord).rgba;
	albedo4 += vec4(vertexColor, 0.0);

	// TODO This kills performance
	if (albedo4.a < 0.5)
	{
		discard;
	}

	// Material properties
	vec3 albedo = pow(albedo4.rgb, vec3(2.2));
	vec3 emissive = texture(pbrTextures[nonuniformEXT(mData.emissive)], texCoord).rgb;
	vec3 tangentNormal = texture(pbrTextures[nonuniformEXT(mData.normal)], texCoord).xyz * 2.0 - 1.0;
	float metallic = texture(pbrTextures[nonuniformEXT(mData.metalness)], texCoord).b;
	float roughness = texture(pbrTextures[nonuniformEXT(mData.roughness)], texCoord).g;
	float ao = texture(pbrTextures[nonuniformEXT(mData.ao)], texCoord).r;
	float alphaRoughness = AlphaDirectLighting(roughness);

	// Input lighting data
	vec3 N = TangentNormalToWorld(tangentNormal, worldPos, normal, texCoord);
	vec3 V = normalize(camUBO.position.xyz - worldPos);
	float NoV = max(dot(N, V), 0.0);

	// Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)  
	vec3 F0 = vec3(pc.baseReflectivity);
	F0 = mix(F0, albedo, metallic);

	// A little bit hacky
	//vec3 Lo = vec3(0.0); // Original code
	vec3 Lo =  albedo * pc.albedoMultipler;

	for (int i = 0; i < lights.length(); ++i)
	{
		LightData light = lights[i];

		Lo += Radiance(
			albedo,
			N,
			V,
			F0,
			metallic,
			roughness,
			alphaRoughness,
			NoV,
			light);
	}

	vec3 ambient = Ambient(
		albedo,
		F0,
		N,
		V,
		metallic,
		roughness,
		ao,
		NoV);

	vec3 color = ambient + emissive + Lo;

	fragColor = vec4(color, 1.0);
}