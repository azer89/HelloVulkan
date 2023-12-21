# version 460 core

#define MATH_PI 3.1415926535897932384626433832795
#define MATH_INV_PI (1.0 / MATH_PI)

layout(set = 0, binding = 0) uniform sampler2D equirectTex;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outFace0;
layout(location = 1) out vec4 outFace1;
layout(location = 2) out vec4 outFace2;
layout(location = 3) out vec4 outFace3;
layout(location = 4) out vec4 outFace4;
layout(location = 5) out vec4 outFace5;

void writeFace(int face, vec3 colorIn)
{
	vec4 color = vec4(colorIn.rgb, 1.0f);

	if (face == 0)
	{
		outFace0 = color;
	}
	else if (face == 1)
	{
		outFace1 = color;
	}
	else if (face == 2)
	{
		outFace2 = color;
	}
	else if (face == 3)
	{
		outFace3 = color;
	}
	else if (face == 4)
	{
		outFace4 = color;
	}
	else
	{
		outFace5 = color;
	}
}

vec3 uvToXYZ(int face, vec2 uv)
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

vec2 dirToUV(vec3 dir)
{
	return vec2(
		0.5f + 0.5f * atan(dir.z, dir.x) / MATH_PI,
		1.f - acos(dir.y) / MATH_PI);
}

void main()
{
	for (int face = 0; face < 6; ++face)
	{
		vec2 texCoordNew = texCoord * 2.0 - 1.0;
		vec3 scan = uvToXYZ(face, texCoordNew);
		vec3 direction = normalize(scan);
		vec2 src = dirToUV(direction);

		writeFace(face, texture(equirectTex, src).rgb);
	}
}