#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

//layout(early_fragment_tests) in;

/*
ClusteredForward/Scene.frag 

Fragment shader for PBR+IBL, naive forward shading (non clustered)
*/

// Include files
#include <CameraUBO.glsl>
#include <LightData.glsl>
#include <PBRHeader.glsl>
#include <PBRPushConstants.glsl>
#include <Hammersley.glsl>
#include <TangentNormalToWorld.glsl>
#include <ClusteredForward/Header.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/BDA.glsl>

// Specialization constant
layout (constant_id = 0) const uint ALPHA_DISCARD = 1;

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 vertexColor;
layout(location = 4) in flat uint meshIndex;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PC { PBRPushConstant pc; };

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; }; // UBO
layout(set = 0, binding = 2) uniform BDABlock { BDA bda; }; // UBO
layout(set = 0, binding = 3) uniform CFUBO { ClusterForwardUBO cfUBO; }; // UBO
layout(set = 0, binding = 4) readonly buffer Lights { LightData lights []; }; // SSBO
layout(set = 0, binding = 5) readonly buffer LightCells { LightCell lightCells []; }; // SSBO
layout(set = 0, binding = 6) readonly buffer LightIndices { uint lightIndices []; }; // SSBO

layout(set = 0, binding = 7) uniform samplerCube specularMap;
layout(set = 0, binding = 8) uniform samplerCube diffuseMap;
layout(set = 0, binding = 9) uniform sampler2D brdfLUT;

layout(set = 0, binding = 10) uniform sampler2D pbrTextures[] ;

#include <ClusteredForward/Radiance.glsl>
#include <Ambient.glsl>

// stackoverflow.com/questions/51108596/linearize-depth
float LinearDepth(float z, float near, float far)
{
	return near * far / (far + z * (near - far));
}

void main()
{
	// Clustered Forward
	// Must use GLM_FORCE_DEPTH_ZERO_TO_ONE preprocessor, or the code below will fail
	float linDepth = LinearDepth(gl_FragCoord.z, cfUBO.cameraNear, cfUBO.cameraFar);
	uint zIndex = uint(max(log2(linDepth) * cfUBO.sliceScaling + cfUBO.sliceBias, 0.0));
	vec2 tileSize =
		vec2(cfUBO.screenSize.x / float(cfUBO.sliceCountX),
			 cfUBO.screenSize.y / float(cfUBO.sliceCountY));
	uvec3 cluster = uvec3(
		gl_FragCoord.x / tileSize.x,
		gl_FragCoord.y / tileSize.y,
		zIndex);
	uint clusterIdx =
		cluster.x +
		cluster.y * cfUBO.sliceCountX +
		cluster.z * cfUBO.sliceCountX * cfUBO.sliceCountY;
	uint lightCount = lightCells[clusterIdx].count;
	uint lightIndexOffset = lightCells[clusterIdx].offset;

	// Buffer device address
	MeshData mData = bda.meshReference.meshes[meshIndex];

	// PBR + IBL
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

	// PBR + IBL, Material properties
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
	vec3 Lo = albedo * pc.albedoMultipler;

	for (int i = 0; i < lightCount; ++i)
	{
		uint lightIndex = lightIndices[i + lightIndexOffset];
		LightData light = lights[lightIndex];

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