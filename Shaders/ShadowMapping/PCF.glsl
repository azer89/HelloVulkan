float ShadowPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = shadowUBO.pcfScale;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	vec3 N = normalize(normal);
	vec3 lightDir = normalize(shadowUBO.lightPosition.xyz - worldPos);
	float bias = max(shadowUBO.shadowMaxBias * (1.0 - dot(N, lightDir)), shadowUBO.shadowMinBias);

	// TODO Very slow, consider change to a constant for better performance
	//const int range = 2;
	//int range = int(shadowUBO.pcfIteration);
	float shadow = 0.0;
	int count = 0;

	// NOTE Loops below use specialization constant
	for (int x = -PCF_ITERATION; x <= PCF_ITERATION; x++)
	{
		for (int y = -PCF_ITERATION; y <= PCF_ITERATION; y++)
		{
			vec2 off = vec2(dx * x, dy * y);
			float dist = texture(shadowMap, sc.st + off).r;
			shadow += (sc.z - bias) > dist ? 0.1 : 1.0;
			count++;
		}
	}
	return shadow / count;
}