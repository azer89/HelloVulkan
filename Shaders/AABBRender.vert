#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 cubeColor;

layout(set = 0, binding = 0) uniform PerFrameUBO
{
	mat4 cameraProjection;
	mat4 cameraView;
	vec4 cameraPosition;
}
frameUBO;

layout(set = 0, binding = 1) uniform InverseViewUBO
{
	mat4 cameraInverseView;
}
invUBO;

struct AABB
{
	vec4 minPoint;
	vec4 maxPoint;
};
layout(set = 0, binding = 2) buffer Clusters
{
	AABB data[];
}
clusters;

const uint INDICES[36] = uint[]
(
	0, 6, 7,
	0, 7, 1,

	0, 1, 2,
	0, 2, 3,
	6, 5, 4,
	6, 4, 7,
	1, 7, 4,
	1, 4, 2,
	0, 3, 5,
	0, 5, 6,
	
	2, 4, 5,
	2, 5, 3
);

void main()
{
	uint vIndex = INDICES[gl_VertexIndex];

	//uint offset = 16 * 12;

	vec4 minPt = clusters.data[gl_InstanceIndex].minPoint;
	vec4 maxPt = clusters.data[gl_InstanceIndex].maxPoint;

	vec4 vView = vec4(1.0, 1.0, 1.0, 1.0);

	if (vIndex == 0)
	{
		vView = vec4(minPt.xyz, 1.0);
	}
	else if (vIndex == 1)
	{
		vView = vec4(maxPt.x, minPt.y, minPt.z, 1.0);
	}
	else if (vIndex == 2)
	{
		vView = vec4(maxPt.x, minPt.y, maxPt.z, 1.0);
	}
	else if (vIndex == 3)
	{
		vView = vec4(minPt.x, minPt.y, maxPt.z, 1.0);
	}
	else if (vIndex == 4)
	{
		vView = vec4(maxPt.xyz, 1.0);
	}
	else if (vIndex == 5)
	{
		vView = vec4(minPt.x, maxPt.y, maxPt.z, 1.0);
	}
	else if (vIndex == 6)
	{
		vView = vec4(minPt.x, maxPt.y, minPt.z, 1.0);
	}
	else // 7
	{
		vView = vec4(maxPt.x, maxPt.y, minPt.z, 1.0);
	}

	vec4 vWorld = invUBO.cameraInverseView * vView;
	//float offset = floor(gl_InstanceIndex / (16.0 * 12.0));
	//vWorld.z -= (0.02 * offset);

	vec4 fragPos = frameUBO.cameraProjection * frameUBO.cameraView * vWorld;

	if (mod(gl_InstanceIndex, 3) == 0)
	{
		cubeColor = vec4(1.0, 0.0, 0.0, 0.9);
	}
	else if (mod(gl_InstanceIndex, 3) == 1)
	{
		cubeColor = vec4(1.0, 1.0, 1.0, 0.9);
	}
	else
	{
		cubeColor = vec4(0.0, 0.0, 1.0, 0.9);
	}

	//fragPos.z += (0.02 * gl_InstanceIndex);

	gl_Position = fragPos;
}