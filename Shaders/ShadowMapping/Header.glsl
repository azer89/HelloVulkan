float TextureProj(vec4 shadowCoord, vec2 off)
{
	vec3 N = normalize(normal);
	vec3 lightDir = normalize(shadowUBO.lightPosition.xyz - worldPos);
	float bias = max(shadowUBO.shadowMaxBias * (1.0 - dot(N, lightDir)), shadowUBO.shadowMinBias);

	float shadow = 1.0;
	if (shadowCoord.z >= 0.0 && shadowCoord.z <= 1.0)
	{
		float dist = texture(shadowMap, shadowCoord.st + off).r;
		if (shadowCoord.w > 0.0 && dist < (shadowCoord.z - bias))
		{
			shadow = 0.1;
		}
	}
	return shadow;
}

float FilterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = shadowUBO.pcfScale;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = int(shadowUBO.pcfIteration);

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += TextureProj(sc, vec2(dx*x, dy*y));
			count++;
		}

	}
	return shadowFactor / count;
}