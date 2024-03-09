#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include <Bindless//VertexData.glsl>

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 3) readonly buffer Vertices { VertexData vertices []; }; // SSBO
layout(set = 0, binding = 4) readonly buffer Indices { uint indices []; }; // SSBO

void main()
{
	//uint index = gl_PrimitiveID;
	ivec3 index = ivec3(indices[3 * gl_PrimitiveID],
		indices[3 * gl_PrimitiveID + 1],
		indices[3 * gl_PrimitiveID + 2]);

	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	hitValue = barycentricCoords;
}
