# version 460 core

#define MATH_PI 3.1415926535897932384626433832795
#define MATH_INV_PI (1.0 / MATH_PI)

layout(set = 0, binding = 1) uniform samplerCube cubeMap;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outFace0;
layout(location = 1) out vec4 outFace1;
layout(location = 2) out vec4 outFace2;
layout(location = 3) out vec4 outFace3;
layout(location = 4) out vec4 outFace4;
layout(location = 5) out vec4 outFace5;

layout(push_constant) uniform FilterParameters
{
	float roughness;
	uint sampleCount;
	uint currentMipLevel;
	uint width;
	float lodBias;
	uint distribution; // enum
}
pFilterParameters;

vec3 filterColor(vec3 N)
{
	// TODO
	return vec3(0.0, 1.0, 0.0);
}

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

void main()
{
	vec2 texCoordNew = texCoord * float(1 << (pFilterParameters.currentMipLevel));
	texCoordNew = texCoordNew * 2.0 - 1.0;

	for (int face = 0; face < 6; ++face)
	{	
		vec3 scan = uvToXYZ(face, texCoordNew);
		vec3 direction = normalize(scan);

		writeFace(face, filterColor(direction));
	}
}