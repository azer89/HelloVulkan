#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#include <Raytracing/Header/RaytracingUBO.glsl>
#include <Raytracing/Header/RayPayload.glsl>
#include <Raytracing/Header/Random.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/BDA.glsl>
#include <MaterialType.glsl>
#include <ModelUBO.glsl>

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 1) rayPayloadEXT bool shadowed;

hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 3) uniform RTUBO { RaytracingUBO ubo; };
layout(set = 0, binding = 4) uniform BDABlock { BDA bda; }; // Buffer device address
layout(set = 0, binding = 5) readonly buffer ModelUBOs { ModelUBO modelUBOs []; };
layout(set = 0, binding = 6) uniform sampler2D pbrTextures[] ;

#include <Raytracing/Header/Triangle.glsl>

vec3 Reflect(vec3 V, vec3 N);
RayPayload Scatter(MeshData mData, Triangle tri, vec3 direction, float t, float shadowFactor, uint seed);

void main()
{
	// Shadow casting
	vec3 lightVector = normalize(ubo.shadowCasterPosition.xyz);
	
	float tmin = 0.001;
	float tmax = 10000.0;
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	shadowed = true;
	float shadowFactor = 1.0;
	traceRayEXT(tlas, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 1);
	if (shadowed)
	{
		shadowFactor = 0.5;
	}

	// Payload
	Triangle tri = GetTriangle(gl_PrimitiveID, gl_GeometryIndexEXT);
	MeshData mData = bda.meshReference.meshes[gl_GeometryIndexEXT];
	rayPayload = Scatter(mData, tri, gl_WorldRayDirectionEXT, gl_HitTEXT, shadowFactor, rayPayload.randomSeed);
}

RayPayload Scatter(MeshData mData, Triangle tri, vec3 direction, float t, float shadowFactor, uint seed)
{
	RayPayload payload;
	payload.distance = t;
	vec4 color = texture(pbrTextures[nonuniformEXT(mData.albedo)], tri.uv);

	if (mData.material == MAT_SPECULAR)
	{
		payload.color = color.xyz + tri.color.xyz; // Add texture color with vertex color
		payload.color *= shadowFactor;
		payload.scatterDir = reflect(direction, tri.normal) + 
			RandomInUnitSphere(seed) * ubo.specularFuzziness; // Specular
		payload.doScatter = true;
	}
	else if (mData.material == MAT_LIGHT)
	{
		payload.color = color.xyz + tri.color.xyz * ubo.lightIntensity;
		payload.doScatter = false;
	}
	else // Lambertian
	{
		payload.color = color.xyz + tri.color.xyz; // Add texture color with vertex color
		payload.color *= shadowFactor;
		payload.scatterDir = tri.normal  + normalize(RandomInUnitSphere(seed));
		payload.doScatter = true;
		
	}

	payload.randomSeed = seed;
	return payload;
}
