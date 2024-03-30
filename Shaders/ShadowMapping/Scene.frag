#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

// www.khronos.org/opengl/wiki/Early_Fragment_Test 
//layout(early_fragment_tests) in;

/*
ShadowMapping/Scene.frag 

Fragment shader for 
	* PBR+IBL
	* Naive forward shading (non clustered)
	* Shadow mapping
	* Bindless textures
	* Buffer device address
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
#include <ShadowMapping/UBO.glsl>
#include <Bindless/BDA.glsl>

// Specialization constant
layout (constant_id = 0) const uint ALPHA_DISCARD = 1;

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 shadowPos;
layout(location = 4) in flat uint meshIndex;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PC { PBRPushConstant pc; };

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; }; // UBO
layout(set = 0, binding = 1) uniform UBOBlock { ShadowUBO shadowUBO; }; // UBO
layout(set = 0, binding = 3) uniform BDABlock { BDA bda; };
layout(set = 0, binding = 4) readonly buffer Lights { LightData lights []; }; // SSBO
layout(set = 0, binding = 5) uniform samplerCube specularMap;
layout(set = 0, binding = 6) uniform samplerCube diffuseMap;
layout(set = 0, binding = 7) uniform sampler2D brdfLUT;
layout(set = 0, binding = 8) uniform sampler2D shadowMap;

// NOTE This requires descriptor indexing feature
layout(set = 0, binding = 9) uniform sampler2D pbrTextures[];

// PCF or Poisson
#include <ShadowMapping/Shadow.glsl>

// PBR and IBL
#include <Radiance.glsl>
#include <Ambient.glsl>

void main()
{
	// This uses buffer device address
	MeshData mData = bda.meshReference.meshes[meshIndex];

	vec4 albedo4 = texture(pbrTextures[nonuniformEXT(mData.albedo)], texCoord).rgba;

	// A performance trick by using a use specialization constant
	// so this part will be removed if material type is not transparent
	if (ALPHA_DISCARD > 0)
	{
		if (albedo4.a < 0.5)
		{
			discard;
		}
	}

	// Material properties
	vec3 albedo = pow(albedo4.rgb, vec3(2.2));
	vec3 emissive = texture(pbrTextures[nonuniformEXT(mData.emissive)], texCoord).rgb;
	vec3 tangentNormal = texture(pbrTextures[nonuniformEXT(mData.normal)], texCoord).xyz;
	float metallic = texture(pbrTextures[nonuniformEXT(mData.metalness)], texCoord).b;
	float roughness = texture(pbrTextures[nonuniformEXT(mData.roughness)], texCoord).g;
	float ao = texture(pbrTextures[nonuniformEXT(mData.ao)], texCoord).r;
	float alphaRoughness = AlphaDirectLighting(roughness);

	vec3 N = normal;
	if (length(tangentNormal) > 0)
	{
		tangentNormal = tangentNormal * 2.0 - 1.0;
		N = TangentNormalToWorld(tangentNormal, worldPos, normal, texCoord);
	}

	vec3 V = normalize(camUBO.position.xyz - worldPos);
	float NoV = max(dot(N, V), 0.0);

	// Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)  
	vec3 F0 = vec3(pc.baseReflectivity);
	F0 = mix(F0, albedo, metallic);

	// A little bit hacky
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

	float shadow = ShadowPoisson(shadowPos / shadowPos.w);

	vec3 color = ambient + emissive + (Lo * shadow);

	fragColor = vec4(color, 1.0);
}