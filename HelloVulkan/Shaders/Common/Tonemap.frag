#version 460 core

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D colorImage;

#define Clamp01(x) clamp(x, 0.0, 1.0)

vec3 Reinhard(vec3 color);
vec3 ACES(vec3 x);
vec3 Filmic(vec3 x);
vec3 PartialUncharted2(vec3 x);
vec3 Uncharted2(vec3 v);
vec3 GammaCorrection(vec3 color);

void main()
{
	vec3 color = texture(colorImage, texCoord).rgb;
	vec3 mappedColor = Reinhard(color);
	mappedColor = GammaCorrection(mappedColor);
	fragColor = vec4(mappedColor, 1.0);
}

vec3 Reinhard(vec3 color)
{
	const float pureWhite = 1.0;

	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance =
		(luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances
	return (mappedLuminance / luminance) * color;
}

vec3 ACES(vec3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return Clamp01((x * (a * x + b)) / (x * (c * x + d) + e));
}

vec3 Filmic(vec3 x)
{
	vec3 X = max(vec3(0.0), x - 0.004);
	vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
	return pow(result, vec3(2.2));
}

vec3 PartialUncharted2(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Uncharted2(vec3 v)
{
	const float exposureBias = 2.0;
	vec3 curr = PartialUncharted2(v * exposureBias);

	vec3 W = vec3(11.2);
	const vec3 whiteScale = vec3(1.0) / PartialUncharted2(W);
	return curr * whiteScale;
}

vec3 GammaCorrection(vec3 color)
{
	const float gamma = 2.2;
	return pow(color, vec3(1.0 / gamma));
}