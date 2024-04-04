#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : require

#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 3) readonly buffer Vertices { VertexData vertices []; };
layout(set = 0, binding = 4) readonly buffer Indices { uint indices []; };
layout(set = 0, binding = 5) readonly buffer MeshDataArray { MeshData meshdataArray[]; };
layout(set = 0, binding = 6) uniform sampler2D pbrTextures[] ;

const vec3 LIGHT_POS = vec3(-1.5, 0.7, 1.5);
const vec3 DEFAULT_COLOR = vec3(0.447, 0.741, 0.639);

#include <Raytracing/Triangle.glsl>

void main()
{
	Triangle tri = GetTriangle(gl_PrimitiveID, gl_GeometryIndexEXT);
	vec3 L = normalize(LIGHT_POS);
	float NoL = max(dot(tri.normal, L), 0.2);

	MeshData mData = meshdataArray[gl_GeometryIndexEXT];
	vec4 albedo4 = texture(pbrTextures[nonuniformEXT(mData.albedo)], tri.uv).rgba;

	hitValue = albedo4.xyz * NoL;
}
