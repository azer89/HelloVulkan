#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#include <Raytracing/RaytracingUBO.glsl>
#include <Raytracing/RayPayload.glsl>
#include <Raytracing/Random.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/BDA.glsl>
#include <LightData.glsl>
#include <ModelUBO.glsl>

//layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
//layout(location = 2) rayPayloadEXT bool shadowed;

hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 3) uniform RTUBO { RaytracingUBO ubo; };
layout(set = 0, binding = 4) uniform BDABlock { BDA bda; }; // Buffer device address
layout(set = 0, binding = 5) readonly buffer Lights { LightData lights []; };
layout(set = 0, binding = 6) readonly buffer ModelUBOs { ModelUBO modelUBOs []; };
layout(set = 0, binding = 7) uniform sampler2D pbrTextures[] ;

#include <Raytracing/Triangle.glsl>

vec3 Reflect(vec3 V, vec3 N)
{
	return V - 2.0 * dot(V, N) * N;
}

RayPayload Scatter(MeshData mData, Triangle tri, vec3 direction, float t, uint seed)
{
	RayPayload payload;
	payload.distance = t;

	// Lambertian
	vec4 color = texture(pbrTextures[nonuniformEXT(mData.albedo)], tri.uv);
	payload.color = color.xyz + tri.color.xyz;
	//payload.scatterDir = tri.normal  + normalize(RandomInUnitSphere(seed)) * 0.1f;
	payload.scatterDir = Reflect(direction, tri.normal)  + normalize(RandomInUnitSphere(seed)) * 0.75;
	payload.doScatter = true;
	payload.randomSeed = seed;

	return payload;
}

// Blinn-Phong
const float LINEAR = 2.9f;
const float QUADRATIC = 3.8f;

void main()
{
	Triangle tri = GetTriangle(gl_PrimitiveID, gl_GeometryIndexEXT);

	MeshData mData = bda.meshReference.meshes[gl_GeometryIndexEXT];

	// RayPayload Scatter(MeshData mData, Triangle tri, vec3 direction, float t, uint seed)
	rayPayload = Scatter(mData, tri, gl_WorldRayDirectionEXT, gl_HitTEXT, rayPayload.randomSeed);
}
