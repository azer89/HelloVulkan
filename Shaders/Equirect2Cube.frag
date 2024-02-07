#version 460 core

/*
Fragment shader to turn an equirectangular image to a cubemap 

This shader is based on:
[1] https://www.turais.de/how-to-load-hdri-as-a-cubemap-in-opengl/
[2] https://en.wikipedia.org/wiki/Cube_mapping#Memory_addressing
*/

#define PI 3.1415926535897932384626433832795

layout(location = 0) in vec2 texCoord;

// Multiple render targets
layout(location = 0) out vec4 cubeFace0;
layout(location = 1) out vec4 cubeFace1;
layout(location = 2) out vec4 cubeFace2;
layout(location = 3) out vec4 cubeFace3;
layout(location = 4) out vec4 cubeFace4;
layout(location = 5) out vec4 cubeFace5;

layout(set = 0, binding = 0) uniform sampler2D envMap;

void WriteFace(int face, vec3 colorIn)
{
	vec4 color = vec4(colorIn.rgb, 1.0f);

	if (face == 0)
	{
		cubeFace0 = color;
	}
	else if (face == 1)
	{
		cubeFace1 = color;
	}
	else if (face == 2)
	{
		cubeFace2 = color;
	}
	else if (face == 3)
	{
		cubeFace3 = color;
	}
	else if (face == 4)
	{
		cubeFace4 = color;
	}
	else
	{
		cubeFace5 = color;
	}
}

// https://en.wikipedia.org/wiki/Cube_mapping#Memory_addressing
vec3 UVToXYZ(int face, vec2 uv)
{
	if (face == 0)
	{ 
		return vec3(1.f, uv.y, -uv.x);
	}
	else if (face == 1)
	{ 
		return vec3(-1.f, uv.y, uv.x);
	}
	else if (face == 2)
	{ 
		return vec3(+uv.x, -1.f, +uv.y);
	}
	else if (face == 3)
	{ 
		return vec3(+uv.x, 1.f, -uv.y);
	}
	else if (face == 4)
	{ 
		return vec3(+uv.x, uv.y, 1.f);
	}
	else 
	{ 
		return vec3(-uv.x, +uv.y, -1.f); 
	}
}

// Convert Cartesian direction vector to spherical coordinates.
vec2 DirToUV(vec3 dir)
{
	return vec2
	(
		0.5f + 0.5f * atan(dir.z, dir.x) / PI, // phi
		1.f - acos(dir.y) / PI // theta
	);
}

void main()
{
	vec2 texCoordTemp = texCoord * 2.0 - 1.0;

	for (int face = 0; face < 6; ++face)
	{
		vec3 position = UVToXYZ(face, texCoordTemp);
		vec3 direction = normalize(position);
		direction.y = -direction.y;
		vec2 finalTexCoord = DirToUV(direction);
		WriteFace(face, texture(envMap, finalTexCoord).rgb);
	}
}