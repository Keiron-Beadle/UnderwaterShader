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
	matrix vertCoralModel;
	matrix geomCoralModel;
	matrix model;
}

struct GSOutput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

static float size = 1.0;
static int NUM_OF_FISH = 64;
static float RADIUS = 0.04;

[maxvertexcount(6)]
void main(
	point float4 input[1] : SV_POSITION,
	inout TriangleStream< GSOutput > output,
	uint primID : SV_PrimitiveID
)
{
	float theta = 2 * 3.14 * primID / NUM_OF_FISH * time.y * 2;
	input[0].xyz += float3(RADIUS * cos(theta) - 0.03,sin(time.y * 2)*0.001, RADIUS * sin(theta));

	float4 pos = mul(input[0], model);

	pos = mul(pos, view);
	pos = mul(pos, projection);

	//billboard vectors
	float4 right = float4(0.01, 0, 0, 0);
	float4 up = float4(0, 0.01, 0, 0);

	float3 v0 = pos.xyz - 0.5 * right.xyz - 0.5 * up.xyz;
	float3 v1 = pos.xyz + 0.5 * right.xyz - 0.5 * up.xyz;
	float3 v2 = pos.xyz + 0.5 * right.xyz + 0.5 * up.xyz;
	float3 v3 = pos.xyz - 0.5 * right.xyz + 0.5 * up.xyz;

	GSOutput outputVertex;

	outputVertex.pos = float4(v0, 1);
	outputVertex.tex = float2(0, 0);
	output.Append(outputVertex);

	outputVertex.pos = float4(v1, 1);
	outputVertex.tex = float2(1, 0);
	output.Append(outputVertex);

	outputVertex.pos = float4(v2, 1);
	outputVertex.tex = float2(1, 1);
	output.Append(outputVertex);

	outputVertex.pos = float4(v0, 1);
	outputVertex.tex = float2(0, 0);
	output.Append(outputVertex);

	outputVertex.pos = float4(v2, 1);
	outputVertex.tex = float2(1, 1);
	output.Append(outputVertex);

	outputVertex.pos = float4(v3, 1);
	outputVertex.tex = float2(0, 1);
	output.Append(outputVertex);

	output.RestartStrip();

}