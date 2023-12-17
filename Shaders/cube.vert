﻿# version 460 core

layout(location = 0) out vec3 direction;

/*layout(binding = 0) uniform CameraUBO
{
	mat4 mvp;
}
camUBO;*/
layout(binding = 0) uniform PerFrameUBO
{
	mat4 cameraProjection;
	mat4 cameraView;
	mat4 model;
	vec4 cameraPosition;
} ubo;


const vec3 pos[8] = vec3[8](
	vec3(-1.0, -1.0, 1.0),
	vec3(1.0,  -1.0, 1.0),
	vec3(1.0,   1.0, 1.0),
	vec3(-1.0,  1.0, 1.0),

	vec3(-1.0, -1.0, -1.0),
	vec3(1.0,  -1.0, -1.0),
	vec3(1.0,   1.0, -1.0),
	vec3(-1.0,  1.0, -1.0)
);

const int indices[36] = int[36]
(
	// Front
	0, 1, 2, 2, 3, 0,
	// Right
	1, 5, 6, 6, 2, 1,
	// Back
	7, 6, 5, 5, 4, 7,
	// Left
	4, 0, 3, 3, 7, 4,
	// Bottom
	4, 5, 1, 1, 0, 4,
	// Top
	3, 2, 6, 6, 7, 3
);

void main()
{
	const float cubeSize = 20.0;
	int idx = indices[gl_VertexIndex];
	
	mat4 mvp = ubo.cameraProjection * ubo.cameraView * ubo.model;
	vec4 position = vec4(cubeSize * pos[idx], 1.0);
	gl_Position = mvp * position;
	
	direction = pos[idx].xyz;
}
