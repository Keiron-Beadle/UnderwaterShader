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

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

struct GeometryShaderInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

GeometryShaderInput main(VertexShaderInput input)
{
	GeometryShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	output.pos = pos;
	output.normal = input.normal;
	output.texcoord = input.texcoord;
	return output;
}
