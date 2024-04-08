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

hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 3) uniform RTUBO { RaytracingUBO ubo; };
layout(set = 0, binding = 4) uniform BDABlock { BDA bda; }; // Buffer device address
layout(set = 0, binding = 5) readonly buffer ModelUBOs { ModelUBO modelUBOs []; };
layout(set = 0, binding = 6) uniform sampler2D pbrTextures[] ;

#include <Raytracing/Header/Triangle.glsl>

vec3 Reflect(vec3 V, vec3 N)
{
	return V - 2.0 * dot(V, N) * N;
}

RayPayload Scatter(MeshData mData, Triangle tri, vec3 direction, float t, uint seed)
{
	RayPayload payload;
	payload.distance = t;

	vec4 color = texture(pbrTextures[nonuniformEXT(mData.albedo)], tri.uv);
	

	if (mData.material == MAT_SPECULAR)
	{
		payload.color = color.xyz + tri.color.xyz; // Add texture color with vertex color
		payload.scatterDir = Reflect(direction, tri.normal)  + normalize(RandomInUnitSphere(seed)) * 0.2; // Specular
		payload.doScatter = true;
	}
	else if (mData.material == MAT_LIGHT)
	{
		payload.color = color.xyz + tri.color.xyz * ubo.lightIntensity;
		payload.doScatter = false;
	}
	else
	{
		payload.color = color.xyz + tri.color.xyz; // Add texture color with vertex color
		payload.scatterDir = Reflect(direction, tri.normal)  + normalize(RandomInUnitSphere(seed)) * 0.85;
		payload.doScatter = true;
		//payload.scatterDir = tri.normal  + normalize(RandomInUnitSphere(seed)); // Lambertian
	}
	
	
	payload.randomSeed = seed;

	return payload;
}

void main()
{
	Triangle tri = GetTriangle(gl_PrimitiveID, gl_GeometryIndexEXT);
	MeshData mData = bda.meshReference.meshes[gl_GeometryIndexEXT];
	rayPayload = Scatter(mData, tri, gl_WorldRayDirectionEXT, gl_HitTEXT, rayPayload.randomSeed);
}
