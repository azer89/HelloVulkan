#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include <Bindless/VertexData.glsl>

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 3) readonly buffer Vertices { VertexData vertices []; }; // SSBO
layout(set = 0, binding = 4) readonly buffer Indices { uint indices []; }; // SSBO

const vec3 LIGHT_POS = vec3(-1.5, 0.7, 1.5);
const vec3 DEFAULT_COLOR = vec3(0.447, 0.741, 0.639);

void main()
{
	ivec3 index = ivec3(indices[3 * gl_PrimitiveID],
		indices[3 * gl_PrimitiveID + 1],
		indices[3 * gl_PrimitiveID + 2]);

	VertexData v0 = vertices[index.x];
	VertexData v1 = vertices[index.y];
	VertexData v2 = vertices[index.z];

	const vec3 bary = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 N = normalize(
		v0.normal.xyz * bary.x + 
		v1.normal.xyz * bary.y + 
		v2.normal.xyz * bary.z);

	vec3 L = normalize(LIGHT_POS);
	float NoL = max(dot(N, L), 0.2);
	hitValue = DEFAULT_COLOR * NoL;
}
