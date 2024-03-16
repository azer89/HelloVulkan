#version 460 core

/*
SlotBased/Mesh.frag
 
Fragment shader for PBR+IBL, naive forward shading (non clustered)
*/

// Include files
#include <LightData.glsl>
#include <CameraUBO.glsl>
#include <PBRHeader.glsl>
#include <PBRPushConstants.glsl>
#include <Hammersley.glsl>
#include <TangentNormalToWorld.glsl>

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PC { PBRPushConstant pc; };

layout(set = 0, binding = 0)  uniform CameraBlock { CameraUBO camUBO; };

layout(set = 0, binding = 2) readonly buffer Lights { LightData lights []; };
layout(set = 0, binding = 3) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 4) uniform sampler2D textureNormal;
layout(set = 0, binding = 5) uniform sampler2D textureMetalness;
layout(set = 0, binding = 6) uniform sampler2D textureRoughness;
layout(set = 0, binding = 7) uniform sampler2D textureAO;
layout(set = 0, binding = 8) uniform sampler2D textureEmissive;
layout(set = 0, binding = 9) uniform samplerCube specularMap;
layout(set = 0, binding = 10) uniform samplerCube diffuseMap;
layout(set = 0, binding = 11) uniform sampler2D brdfLUT;

#include <Radiance.glsl>
#include <Ambient.glsl>

// stackoverflow.com/questions/51108596/linearize-depth
float LinearDepth(float z, float near, float far)
{
	return near * far / (far + z * (near - far));
}

void main()
{
	vec4 albedo4 = texture(textureAlbedo, texCoord).rgba;

	// TODO This kills performance
	if (albedo4.a < 0.5)
	{
		discard;
	}

	// Material properties
	vec3 albedo = pow(albedo4.rgb, vec3(2.2)); 
	vec3 emissive = texture(textureEmissive, texCoord).rgb;
	float metallic = texture(textureMetalness, texCoord).b;
	float roughness = texture(textureRoughness, texCoord).g;
	float ao = texture(textureAO, texCoord).r;

	float alphaRoughness = AlphaDirectLighting(roughness);
	vec3 tangentNormal = texture(textureNormal, texCoord).xyz * 2.0 - 1.0;

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