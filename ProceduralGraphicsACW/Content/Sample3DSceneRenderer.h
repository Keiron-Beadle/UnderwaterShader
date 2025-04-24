#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

namespace ProceduralGraphicsACW
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }


	private:
		void Rotate(float radians);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		DirectX::XMFLOAT4 eye;
		DirectX::XMFLOAT4 lookAt;
		DirectX::XMFLOAT4 misc;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_sdfInputLayout;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_coralVertexInputLayout;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_coralGeomInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexCoralBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_geomCoralBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_geomCoralIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_fishBuffer;
		
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_sdfVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_sdfPixelShader;
		
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vCoralVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_vCoralPixelShader;

		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_gCoralVertexShader;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_gCoralGeomShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_gCoralPixelShader;

		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_fishVertexShader;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_fishGeomShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_fishPixelShader;

		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_modelBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_lightBuffer;

		Microsoft::WRL::ComPtr<ID3D11Resource> m_seafloorTexture;
		Microsoft::WRL::ComPtr<ID3D11Resource> m_coralTexture;
		Microsoft::WRL::ComPtr<ID3D11Resource> m_vertCoralTexture;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_seafloorSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_coralSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_vertCoralSRV;

		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_noDepthState;
		Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendStateAlpha;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_noCullRasterState;

		// System resources for cube geometry.
		MyConstantBuffer	m_constantBufferData;
		ModelConstantBuffer m_modelBufferData;
		LightConstantBuffer m_lightBufferData;
		uint32	m_indexCount;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;
	};
}

