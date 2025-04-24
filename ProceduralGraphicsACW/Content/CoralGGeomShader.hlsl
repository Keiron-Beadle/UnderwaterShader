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
	matrix model;
	matrix fishModel;
}

struct GS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

struct GS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
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

float3 estimateNormal(float3 v1, float3 v2, float3 v3) {
	return normalize(cross(v2 - v1, v3 - v1));
}

float bellSmoothstep(float edge0, float edge1, float x) {
	x = saturate((x - edge0) / (edge1 - edge0));
	return x * x * (3 - 2 * x);
}

[maxvertexcount(15)]
void main(triangle GS_INPUT input[3], uint pid : SV_PrimitiveID, inout TriangleStream<GS_OUTPUT> output)
{
	float4 c = float4(-2, 0.2, -5, 0);
	float4 wp0 = input[0].position + c;
	float4 wp1 = input[1].position + c;
	float4 wp2 = input[2].position + c;

	//
	wp0 = mul(wp0, model);
	wp0 = mul(wp0, view);
	wp0 = mul(wp0, projection);

	wp1 = mul(wp1, model);
	wp1 = mul(wp1, view);
	wp1 = mul(wp1, projection);

	wp2 = mul(wp2, model);
	wp2 = mul(wp2, view);
	wp2 = mul(wp2, projection);
	//

	float mtime = time.z;
	//float extrusion = noise(float2(length(wp0 + wp1 + wp2)*0.04, 0.2*cos(mtime * 3.14)));
	float extrusion = saturate(0.3 - cos(time.x * 0.54) * 0.03);
	//extrusion *= 0.3 + 0.05 * sin(pid * 832.37483 + time * 88.76);
	extrusion *= 1.0 + sin(pid);
	extrusion *= -1;

	float3 offs = estimateNormal(wp0, wp1, wp2) * extrusion;
	float4 wp3 = wp0 + float4(offs, 0);
	float4 wp4 = wp1 + float4(offs, 0);
	float4 wp5 = wp2 + float4(offs, 0);

	float3 wn = estimateNormal(wp3, wp4, wp5);
	float np = saturate(extrusion * 10);
	float3 wn0 = lerp(input[0].normal, wn, np);
	float3 wn1 = lerp(input[1].normal, wn, np);
	float3 wn2 = lerp(input[2].normal, wn, np);

	GS_OUTPUT temp;
	temp.position = wp3 + c; temp.normal = float3(wn0); temp.texcoord = input[0].texcoord;
	output.Append(temp);
	temp.position = wp4 + c; temp.normal = float3(wn1); temp.texcoord = input[1].texcoord;
	output.Append(temp);
	temp.position = wp5 + c; temp.normal = float3(wn2); temp.texcoord = input[2].texcoord;
	output.Append(temp);
	output.RestartStrip();

	wn = estimateNormal(wp3, wp0, wp4);
	temp.position = wp3 + c; temp.normal = float3(wn); temp.texcoord = input[0].texcoord;
	output.Append(temp);
	temp.position = wp0 + c; temp.normal = float3(wn); temp.texcoord = input[1].texcoord;
	output.Append(temp);
	temp.position = wp4 + c; temp.normal = float3(wn); temp.texcoord = input[2].texcoord;
	output.Append(temp);
	temp.position = wp1 + c; temp.normal = float3(wn); temp.texcoord = input[0].texcoord;;
	output.Append(temp);
	output.RestartStrip();

	wn = estimateNormal(wp4, wp1, wp5);
	temp.position = wp4 + c; temp.normal = float3(wn); temp.texcoord = input[0].texcoord; // 1
	output.Append(temp);
	temp.position = wp1 + c; temp.normal = float3(wn); temp.texcoord = input[2].texcoord; // 2
	output.Append(temp);

	//THIS ONE
	temp.position = wp5 + c; temp.normal = float3(wn); temp.texcoord = input[0].texcoord; // 1
	output.Append(temp);
	temp.position = wp2 + c; temp.normal = float3(wn); temp.texcoord = input[1].texcoord; // 0
	output.Append(temp);
	output.RestartStrip();

	wn = estimateNormal(wp5, wp2, wp3);
	temp.position = wp5 + c; temp.normal = float3(wn); temp.texcoord = input[2].texcoord; // 1
	output.Append(temp);
	temp.position = wp2 + c; temp.normal = float3(wn); temp.texcoord = input[1].texcoord; // 0
	output.Append(temp);

	//THIS ONE TOO
	temp.position = wp3 + c; temp.normal = float3(wn); temp.texcoord = input[0].texcoord; // 0
	output.Append(temp);
	temp.position = wp0 + c; temp.normal = float3(wn); temp.texcoord = input[2].texcoord; // 1
	output.Append(temp);
	output.RestartStrip();
}