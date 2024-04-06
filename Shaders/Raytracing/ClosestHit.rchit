#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#include <Raytracing/CameraProperties.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/BDA.glsl>
#include <LightData.glsl>

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 3) uniform Camera { CameraProperties cam; };
layout(set = 0, binding = 4) uniform BDABlock { BDA bda; }; // Buffer device address
layout(set = 0, binding = 5) readonly buffer Lights { LightData lights []; };
layout(set = 0, binding = 6) uniform sampler2D pbrTextures[] ;

#include <Raytracing/Triangle.glsl>

// Blinn-Phong
const float LINEAR = 2.9f;
const float QUADRATIC = 3.8f;

void main()
{
	Triangle tri = GetTriangle(gl_PrimitiveID, gl_GeometryIndexEXT);

	MeshData mData = bda.meshReference.meshes[gl_GeometryIndexEXT];
	vec3 albedo = texture(pbrTextures[nonuniformEXT(mData.albedo)], tri.uv).xyz;
	float specular = 0.299 * albedo.r + 0.587 * albedo.g + 0.114 * albedo.b;

	vec3 color = vec3(0.0);
	vec3 viewDir = normalize(cam.position.xyz - tri.fragPosition);
	for (int i = 0; i < lights.length(); ++i)
	{
		LightData light = lights[i];
		
		// Diffuse
		vec3 lightDir = normalize(light.position.xyz - tri.fragPosition);
		vec3 diffuse = max(dot(tri.normal, lightDir), 0.0) * albedo * light.color.xyz;

		// Specular
		vec3 halfwayDir = normalize(lightDir + viewDir);
		float spec = pow(max(dot(tri.normal, halfwayDir), 0.0), 16.0);
		vec3 specular = light.color.xyz * spec * specular;

		// Attenuation
		float distance = length(light.position.xyz - tri.fragPosition);
		float attenuation = 1.0 / (1.0 + LINEAR * distance + QUADRATIC * distance * distance);

		diffuse *= attenuation;
		specular *= attenuation;

		color += diffuse + specular;
	}

	hitValue = color;
}
