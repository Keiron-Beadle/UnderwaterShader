struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

cbuffer MyConstantBuffer : register(b0)
{
	matrix view;
	matrix projection;
	float4 screenDim;
	float4 _time;
	float4 eye;
	float4 viewDir;
};

cbuffer LightBuffer : register(b2) {
	float4 skyColour;
	float4 sunLightColour;
	float4 skyLightColour;
	float4 indLightColour;
	float4 horizonColour;
	float4 sunDirection;
	float4 godRayColour;
}

sampler samp : register(s0);
Texture2D tex : register(t2);

#define MAX_ITER 5
#define TAU 6.28318530718
#define CAUSTIC_POWER 10.0
#define CAUSTICS_INTENSITY 0.005
#define GOD_RAY_INTENSITY 0.005
#define CAUSTICS_SPEED 0.2

float3 caustic(float2 uv)
{
	float2 p = (fmod(uv * TAU, TAU) - 250.0);
	float time = _time.x * CAUSTICS_SPEED + 23.0;
	float2 i = p;
	float c = 1.0;
	float inten = CAUSTICS_INTENSITY;

	for (int n = 0; n < MAX_ITER; n++)
	{
		float t = time * (1.0 - (3.5 / float(n + 1)));
		i = p + float2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
		c += 1.0 / length(float2(p.x / (sin(i.x + t) / inten), p.y / (cos(i.y + t) / inten)));
	}

	c /= float(MAX_ITER);
	c = 1.17 - pow(c, 1.4);
	float3 colour = pow(abs(c), CAUSTIC_POWER);
	colour = clamp(colour + float3(0.0, 0.35, 0.5), 0.0, 1.0);

	colour = lerp(colour, float3(1.0, 0.2, 0.5), 0.3);

	return colour;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 colour = skyColour;
	float3 sky = 0.0;

	float3 rgb = tex.Sample(samp, input.texcoord).rgb;
	float3 N = input.normal;
	float sun = clamp(dot(sunDirection, N), 0, 1);
	float3 lightDir = sunDirection;
	float diff = max(dot(N, lightDir), 0.0);
	float3 diffuse = diff * rgb;
	diffuse *= caustic(float2(lerp(input.pos.x, input.pos.y, 0.2), lerp(input.pos.z, input.pos.y, 0.2)) * 1.1);
	float3 lightCol = 1.0 * sunLightColour;
	lightCol += 0.7 * sky * skyLightColour;
	colour *= 0.8 * diffuse * lightCol;
	colour = lerp(colour, horizonColour, 1.0 - exp(-0.3 * pow(1.0, 1.0)));
	float3 gamma = (float3).47;
	return float4(pow(colour, gamma), 1);
}
