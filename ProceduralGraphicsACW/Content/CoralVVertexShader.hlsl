cbuffer MyConstantBuffer : register(b0)
{
	matrix view;
	matrix projection;
	float4 screenDim;
	float4 time;
	float4 eye;
	float4 viewDir;
	float4 misc;
};

cbuffer ModelBuffer : register(b1) {
	matrix sdfmodel;
	matrix model;
	matrix geomCoralModel;
	matrix fishModel;
}

#define PI 3.14159265359

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR;
	float3 normal : NORMAL;
};

float hash(float2 grid) {
	float h = dot(grid, float2 (127.1, 311.7));
	return frac(sin(h) * 43758.5453123);
}

float noise(in float2 p) {
	float2 grid = floor(p);
	float2 f = frac(p);
	float2 uv = f * f * (3.0 - 2.0 * f);
	float n1, n2, n3, n4;
	n1 = hash(grid + float2(0, 0));
	n2 = hash(grid + float2(1, 0));
	n3 = hash(grid + float2(0, 1));
	n4 = hash(grid + float2(1, 1));
	n1 = lerp(n1, n2, uv.x);
	n2 = lerp(n3, n4, uv.x);
	n1 = lerp(n1, n2, uv.y);
	return n1;
}

float3 hsv2rgb(float3 c) {
	c = float3(c.x, clamp(c.yz, 0.0, 1.0));
	float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

PixelShaderInput main(float4 position : POSITION0, uint vertexID : SV_VertexID)
{
	float segmentsPerCircle = 64.0;
	float pointsPerCircle = segmentsPerCircle * 2.0;
	float numCircles = floor(misc.x / pointsPerCircle);

	float vID = fmod(vertexID, pointsPerCircle);
	float cID = floor(vertexID / pointsPerCircle);
	float cv = cID / numCircles;

	float u = floor(vID * 0.5) + fmod(vID, 2.0);
	float v = u / segmentsPerCircle;
	float a = v * PI * 2.0 + cv;

	float2 p = float2(cos(a), sin(a));
	float distFromCentre = length(p);
	float s = noise(float2(cv, vID * 8));
	float cvv = cv * 2.0 - 1.0;
	float3 pos = float3(noise(p), p * lerp(0.0, 1.0, sin(cv * PI)) * s);

	//Calculate normal using VID+1
	float vID1 = fmod(vertexID + 1, pointsPerCircle);
	float cID1 = floor((vertexID + 1) / pointsPerCircle);
	float cv1 = cID / numCircles;
	float u1 = floor(vID1 * 0.5) + fmod(vID1, 2.0);
	float v1 = u1 / segmentsPerCircle;
	float a1 = v1 * PI * 2.0 + cv1;
	float2 p1 = float2(cos(a1), sin(a1));
	float distFromCentre1 = length(p1);
	float s1 = noise(float2(cv1, vID1 * 8));
	float cvv1 = cv1 * 2.0 - 1.0;
	float3 pos1 = float3(noise(p1), p1 * lerp(0.0, 1.0, sin(cv1 * PI)) * s1);

	float3 normal = cross(pos, pos1);
	//
	//pos *= 0.5;
	pos = mul(pos, model);
	pos.x = pos.x + 0.3;
	pos.y = pos.y - 0.4;

	pos = mul(pos, view);
	pos = mul(pos, projection);

	PixelShaderInput output;

	output.pos = float4(pos, 1);
	output.normal = normalize(normal);

	float boost = step(0.7, s);
	float hue = (misc.y * 0.01) + pow(s, 3.0) * 0.2 + boost * 0.5;
	float sat = 1.0 - pow(pos.z, 1.0) - pow(s, 4.0);
	float val = lerp(0.25, 2.0, pow(s, 2.0));
	output.color = hsv2rgb(float3(hue, sat, val));
	return output;
}
