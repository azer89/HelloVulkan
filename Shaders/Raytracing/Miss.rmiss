#version 460
#extension GL_EXT_ray_tracing : enable

#include <Raytracing/RayPayload.glsl>
#include <Raytracing/RaytracingUBO.glsl>

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

layout(set = 0, binding = 3) uniform RTUBO { RaytracingUBO ubo; };

void main()
{
	const float t = 0.5 * (normalize(gl_WorldRayDirectionEXT).y + 1.0);
	const vec3 gradientStart = vec3(0.5, 0.6, 1.0);
	const vec3 gradientEnd = vec3(1.0);
	const vec3 skyColor = mix(gradientEnd, gradientStart, t);

	rayPayload.color = skyColor * ubo.skyIntensity;
	rayPayload.distance = -1.0;
}