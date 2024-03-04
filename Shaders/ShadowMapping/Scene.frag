#version 460 core
#extension GL_EXT_nonuniform_qualifier : require

// www.khronos.org/opengl/wiki/Early_Fragment_Test 
//layout(early_fragment_tests) in;

/*
Fragment shader for 
	* PBR+IBL
	* Naive forward shading (non clustered)
	* Shadow mapping
	* Bindless
*/

// Include files
#include <LightData.glsl>
#include <PBRHeader.glsl>
#include <Hammersley.glsl>
#include <TangentNormalToWorld.glsl>
#include <ShadowMapping//UBO.glsl>
#include <Bindless//MeshData.glsl>

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec3 viewPos;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in flat uint meshIndex;

layout(location = 0) out vec4 fragColor;

layout(push_constant)
#include <PBRPushConstants.glsl>

// UBO
layout(set = 0, binding = 0)
#include <CameraUBO.glsl>

// UBO
layout(set = 0, binding = 1) uniform ShadowBlock { ShadowUBO shadowUBO; };

// SSBO
layout(set = 0, binding = 5) readonly buffer Meshes { MeshData meshes []; };

// SSBO
layout(set = 0, binding = 6) readonly buffer Lights { LightData lights []; };

layout(set = 0, binding = 7) uniform samplerCube specularMap;
layout(set = 0, binding = 8) uniform samplerCube diffuseMap;
layout(set = 0, binding = 9) uniform sampler2D brdfLUT;
layout(set = 0, binding = 10) uniform sampler2DArray shadowMap;

// NOTE This requires descriptor indexing feature
layout(set = 0, binding = 11) uniform sampler2D pbrTextures[];

// Shadow mapping functions
#include <ShadowMapping//PCF.glsl>

#include <Radiance.glsl>
#include <Ambient.glsl>

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0);

void main()
{
	MeshData mData = meshes[meshIndex];

	vec4 albedo4 = texture(pbrTextures[nonuniformEXT(mData.albedo)], texCoord).rgba;

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


	// Obtaining the cascadeIndex below is optimized for SHADOW_MAP_CASCADE_COUNT = 4
	//vec4 res = step(viewPos.z, shadowUBO.splitDepths);
	//uint cascadeIndex = uint(res.x + res.y + res.z + res.w);
	uint cascadeIndex = 0;
	for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i)
	{
		if (viewPos.z < shadowUBO.splitDepths[i])
		{
			cascadeIndex = i + 1;
		}
	}

	vec4 shadowPos = biasMat * shadowUBO.lightSpaceMatrices[cascadeIndex] * vec4(worldPos, 1.0);

	float shadow = ShadowPCF(shadowPos / shadowPos.w, cascadeIndex);
	vec3 color = ambient + emissive + (Lo * shadow);
	fragColor = vec4(color, 1.0);

	// For debugging
	/*switch (cascadeIndex)
	{
		case 0: fragColor.rgb *= vec3(1.0f, 0.25f, 0.25f); break;
		case 1: fragColor.rgb *= vec3(0.25f, 1.0f, 0.25f); break;
		case 2: fragColor.rgb *= vec3(0.25f, 0.25f, 1.0f); break;
		case 3: fragColor.rgb *= vec3(1.0f, 0.25f, 0.25f); break;
	}*/
}