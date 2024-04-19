/*
Adapted from
	3D Graphics Rendering Cookbook
	by Sergey Kosarevsky & Viktor Latypov
	github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook
*/

float Log10(float x)
{
	return log(x) / log(10.0);
}

float Saturate1(float x)
{
	return clamp(x, 0.0, 1.0);
}

vec2 Saturate2(vec2 x)
{
	return clamp(x, vec2(0.0), vec2(1.0));
}

float Max2(vec2 v)
{
	return max(v.x, v.y);
}

vec4 GridColor(vec2 uv, vec2 camPos)
{
	vec2 dudv = vec2(
		length(vec2(dFdx(uv.x), dFdy(uv.x))),
		length(vec2(dFdx(uv.y), dFdy(uv.y)))
	);

	float lodLevel = max(0.0, Log10(GRID_CELL_SIZE) + 1.0);
	float lodFade = fract(lodLevel);

	// Cell sizes for lod0, lod1 and lod2
	float lod0 = GRID_CELL_SIZE * pow(10.0, floor(lodLevel));
	float lod1 = lod0 * 10.0;
	float lod2 = lod1 * 10.0;

	// Each anti-aliased line covers up to 4 pixels
	dudv *= 4.0;

	// Update grid coordinates for subsequent alpha calculations (centers each anti-aliased line)
	uv += dudv / 2.0F;

	// Calculate absolute distances to cell line centers for each lod and pick max X/Y to get coverage alpha value
	float lod0a = Max2(vec2(1.0) - abs(Saturate2(mod(uv, lod0) / dudv) * 2.0 - vec2(1.0)));
	float lod1a = Max2(vec2(1.0) - abs(Saturate2(mod(uv, lod1) / dudv) * 2.0 - vec2(1.0)));
	float lod2a = Max2(vec2(1.0) - abs(Saturate2(mod(uv, lod2) / dudv) * 2.0 - vec2(1.0)));

	uv -= camPos;

	// Blend between falloff colors to handle LOD transition
	vec4 c = lod2a > 0.0 ? GRID_COLOR_THICK : lod1a > 0.0 ? 
		mix(GRID_COLOR_THICK, GRID_COLOR_THIN, lodFade) : 
		GRID_COLOR_THIN;

	// Calculate opacity falloff based on distance to grid extents
	float opacityFalloff = (1.0 - Saturate1(length(uv) / GRID_EXTENTS));

	// Blend between LOD level alphas and scale with opacity falloff
	c.a *= (lod2a > 0.0 ? lod2a : lod1a > 0.0 ? lod1a : (lod0a * (1.0 - lodFade))) * opacityFalloff;

	return c;
}
