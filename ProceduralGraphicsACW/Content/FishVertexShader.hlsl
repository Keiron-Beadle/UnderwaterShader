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

float4 main(float4 pos : POSITION) : SV_POSITION
{
//	pos.w = 1;
//pos = mul(pos, model);
//pos = mul(pos, view);
//pos = mul(pos, projection);
return float4(pos.xyz,1);
}