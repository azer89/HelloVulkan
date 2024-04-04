#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : require

#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <LightData.glsl>

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 3) readonly buffer Vertices { VertexData vertices []; };
layout(set = 0, binding = 4) readonly buffer Indices { uint indices []; };
layout(set = 0, binding = 5) readonly buffer MeshDataArray { MeshData meshdataArray[]; };
layout(set = 0, binding = 6) readonly buffer Lights { LightData lights []; };
layout(set = 0, binding = 7) uniform sampler2D pbrTextures[] ;

#include <Raytracing/Triangle.glsl>

void main()
{
	Triangle tri = GetTriangle(gl_PrimitiveID, gl_GeometryIndexEXT);

	MeshData mData = meshdataArray[gl_GeometryIndexEXT];
	vec3 albedo = texture(pbrTextures[nonuniformEXT(mData.albedo)], tri.uv).xyz;

	vec3 color = vec3(0.0);
	for (int i = 0; i < lights.length(); ++i)
	{
		LightData light = lights[i];
		vec3 L = normalize(light.position.xyz);
		float NoL = max(dot(tri.normal, L), 0.2);
		color += albedo * NoL;
	}

	hitValue = color;
}
