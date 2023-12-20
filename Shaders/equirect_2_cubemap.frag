
#version 460 core

#define UX3D_MATH_PI 3.1415926535897932384626433832795
#define UX3D_MATH_INV_PI (1.0 / UX3D_MATH_PI)

layout(set = 0, binding = 0) uniform sampler2D uPanorama;

layout(location = 0) in vec2 inUV;

// output cubemap faces
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
		outFace0 = color;
	else if (face == 1)
		outFace1 = color;
	else if (face == 2)
		outFace2 = color;
	else if (face == 3)
		outFace3 = color;
	else if (face == 4)
		outFace4 = color;
	else //if(face == 5)
		outFace5 = color;
}

vec3 uvToXYZ(int face, vec2 uv)
{
	if (face == 0) // right
	{
		return vec3(1.f, uv.y, -uv.x);
	}
	else if (face == 1) // left
	{
		return vec3(-1.f, uv.y, uv.x);
	}
	else if (face == 2) // top
	{
		return vec3(+uv.x, -1.f, +uv.y);
	}
	else if (face == 3) // bottom
	{
		return vec3(+uv.x, 1.f, -uv.y);
	}
	else if (face == 4) // back
	{
		return vec3(+uv.x, uv.y, 1.f);
	}
	else // front
	{
		return vec3(-uv.x, +uv.y, -1.f);
	}
}

/*vec2 dirToUV(vec3 dir)
{
	return vec2(
			0.5f + 0.5f * atan(dir.z, dir.x) / UX3D_MATH_PI,
			1.f - acos(dir.y) / UX3D_MATH_PI);
}*/

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

// entry point
void main()
{
	for (int face = 0; face < 6; ++face)
	{
		vec3 scan = uvToXYZ(face, inUV*2.0-1.0);

		vec3 direction = normalize(scan);

		//vec2 src = dirToUV(direction);
		vec2 src = SampleSphericalMap(direction);

		writeFace(face, texture(uPanorama, src).rgb);
	}
}