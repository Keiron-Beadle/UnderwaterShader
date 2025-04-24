#pragma once

namespace ProceduralGraphicsACW
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct MyConstantBuffer
	{
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
		DirectX::XMFLOAT4 screenDimensions;
		DirectX::XMFLOAT4 time;
		DirectX::XMFLOAT4 eye;
		DirectX::XMFLOAT4 viewDir;
		DirectX::XMFLOAT4 misc;
	};

	struct ModelConstantBuffer {
		DirectX::XMFLOAT4X4 terrainModel;
		DirectX::XMFLOAT4X4 vertexCoralModel;
		DirectX::XMFLOAT4X4 geometryCoralModel;
		DirectX::XMFLOAT4X4 fishModel;
	};

	//float3 skyColour = float3(0.3, 1.0, 1.0);
	//float3 godRayCol = float3(0.5, 0.7, 1.0);
	//float3 sunLightColour = float3(1.7, 0.65, 0.65);
	//float3 skyLightColour = float3(0.8, 0.35, 0.15);
	//float3 indLightColour = float3(0.04, 0.03, 0.02);
	//float3 horizonColour = float3(0.0, 0.05, 0.2);
	//float3 sunDirection = normalize(float3(-1.0, 1.0, 0.0));

	struct LightConstantBuffer {
		DirectX::XMFLOAT4 skyColour;
		DirectX::XMFLOAT4 sunLightColour;
		DirectX::XMFLOAT4 skyLightColour;
		DirectX::XMFLOAT4 indLightColour;
		DirectX::XMFLOAT4 horizonColour;
		DirectX::XMFLOAT4 sunDirection;
		DirectX::XMFLOAT4 godRayColour;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPosition
	{
		DirectX::XMFLOAT3 pos;
	};

	struct VertexPosNormTex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texcoord;
	};
}