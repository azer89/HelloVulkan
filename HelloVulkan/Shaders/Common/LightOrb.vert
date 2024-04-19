#version 460 core
#extension GL_ARB_separate_shader_objects : enable

#include <CameraUBO.glsl>
#include <LightData.glsl>

layout(location = 0) out vec2 fragOffset;
layout(location = 1) out vec4 circleColor;

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; };
layout(binding = 1) readonly buffer Lights { LightData lights []; };

const vec2 OFFSETS[6] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0,  1.0),
	vec2( 1.0, -1.0),
	vec2( 1.0, -1.0),
	vec2(-1.0,  1.0),
	vec2( 1.0,  1.0)
);
const float RADIUS = 0.1;

void main()
{
	LightData light = lights[gl_InstanceIndex];

	vec3 camRight = vec3(camUBO.view[0][0], camUBO.view[1][0], camUBO.view[2][0]);
	vec3 camUp = vec3(camUBO.view[0][1], camUBO.view[1][1], camUBO.view[2][1]);

	fragOffset = OFFSETS[gl_VertexIndex];

	vec3 positionWorld = light.position.xyz +
		RADIUS * fragOffset.x * camRight +
		RADIUS * fragOffset.y * camUp;

	circleColor = light.color;
	gl_Position = camUBO.projection * camUBO.view * vec4(positionWorld, 1.0);
}