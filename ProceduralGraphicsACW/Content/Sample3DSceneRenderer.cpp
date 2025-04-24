#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"
#include "DDSTextureLoader.h"

using namespace ProceduralGraphicsACW;

using namespace DirectX;
using namespace Windows::Foundation;

static constexpr int NUM_OF_FISH = 64;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);
	XMStoreFloat4(&eye, XMVectorSet(0.5f, 1.4f, 1.0f, 1.0f));
	XMStoreFloat4(&lookAt, XMVectorSet(-0.1f, 1.4f, 0.0f, 1.0f));

	//Number of vertices to use for vertex coral.
	misc.x = 10000;
	// Setup model matrices
	XMStoreFloat4x4(&m_modelBufferData.terrainModel, XMMatrixIdentity());

	//

	//Setup lights
	m_lightBufferData.skyColour = XMFLOAT4(0.4f,0.4f,1.0f,1.0f);
	m_lightBufferData.sunLightColour = XMFLOAT4(1.7f, 0.65f, 0.65f,1.0f);
	m_lightBufferData.skyLightColour = XMFLOAT4(0.8f, 0.35f, 0.15f, 1.0f); // floor colour
	m_lightBufferData.indLightColour = XMFLOAT4(0.04f, 0.03f, 0.02f, 1.0f);
	m_lightBufferData.horizonColour = XMFLOAT4(0.01f, 0.04f, 0.2f, 1.0f);
	m_lightBufferData.godRayColour = XMFLOAT4(0.6f, 0.4f, 0.7f, 1.0f);

	XMStoreFloat4(&m_lightBufferData.sunDirection, 
		XMVector3Normalize(XMVectorSet(4.0f, 1.0f, 0.0f,0.0f)));
	//
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	double time = timer.GetTotalSeconds();
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMVECTOR eyeV = XMLoadFloat4(&eye);
	XMVECTOR lookAtV = XMLoadFloat4(&lookAt);

	m_constantBufferData.eye = eye;
	XMStoreFloat4(&m_constantBufferData.viewDir, lookAtV - eyeV);
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eyeV, lookAtV, up)));

	XMStoreFloat4(&m_constantBufferData.time, XMVectorSet(time, time * 0.1f,
		time * 0.01f, 0));
	XMMATRIX vertexCoralModel = XMMatrixRotationY(7.65) * XMMatrixRotationZ(6.2f) *
		XMMatrixScaling(0.2f, 0.2f, 0.2f);
	XMStoreFloat4x4(&m_modelBufferData.vertexCoralModel, vertexCoralModel);

	XMMATRIX geomCoralModel = XMMatrixTranslation(0,-3.0,0.0);
	XMStoreFloat4x4(&m_modelBufferData.geometryCoralModel, geomCoralModel);

	XMMATRIX fishModel = XMMatrixScaling(2.0f,2.0f,2.0f) * XMMatrixTranslation(0,-4,0);
	XMStoreFloat4x4(&m_modelBufferData.fishModel, fishModel);

	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = time * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	m_constantBufferData.misc = misc;
	context->UpdateSubresource1(m_constantBuffer.Get(), 0, nullptr, &m_constantBufferData, 0, 0, 0);
	context->UpdateSubresource1(m_modelBuffer.Get(), 0, nullptr, &m_modelBufferData, 0, 0, 0);
	context->UpdateSubresource1(m_lightBuffer.Get(), 0, nullptr, &m_lightBufferData, 0, 0, 0);


	// DRAW THE SDF SCENE
	context->OMSetDepthStencilState(nullptr, 0);

	UINT stride = sizeof(VertexPosition);
	UINT offset = 0;

	context->RSSetState(nullptr);
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_sdfInputLayout.Get());
	context->VSSetShader(m_sdfVertexShader.Get(),nullptr,0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->VSSetConstantBuffers1(1,1,m_modelBuffer.GetAddressOf(),nullptr,nullptr);
	context->PSSetShader(m_sdfPixelShader.Get(),nullptr,0);
	context->PSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetConstantBuffers1(1, 1, m_modelBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetConstantBuffers1(2, 1, m_lightBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetConstantBuffers1(1, 1, m_modelBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
	context->PSSetShaderResources(0, 1, m_seafloorSRV.GetAddressOf());
	context->PSSetShaderResources(1, 1, m_coralSRV.GetAddressOf());
	context->PSSetShaderResources(2, 1, m_vertCoralSRV.GetAddressOf());
	float blendFactor[] = { 1,1,1,1 };
	UINT sampleMask = 0xffffffff;
	context->OMSetBlendState(m_blendStateAlpha.Get(), blendFactor, sampleMask);

	context->Draw(6,0);

	//
	// DRAW THE EXPLICIT RENDERABLES
	
	//using same input layout as sdf draw as fish only need pos data.
	context->OMSetDepthStencilState(m_noDepthState.Get(), 0);
	context->IASetVertexBuffers(0, 1, m_fishBuffer.GetAddressOf(), &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->VSSetShader(m_fishVertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_fishPixelShader.Get(), nullptr, 0);
	context->GSSetShader(m_fishGeomShader.Get(), nullptr, 0);
	context->Draw(NUM_OF_FISH, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->OMSetBlendState(nullptr, blendFactor, sampleMask);
	//

	UINT geomStride = sizeof(VertexPosNormTex);
	context->IASetVertexBuffers(0, 1, m_geomCoralBuffer.GetAddressOf(), &geomStride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetIndexBuffer(m_geomCoralIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetInputLayout(m_coralGeomInputLayout.Get());
	context->VSSetShader(m_gCoralVertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_gCoralPixelShader.Get(), nullptr, 0);
	context->GSSetShader(m_gCoralGeomShader.Get(), nullptr, 0);
	context->DrawIndexed(m_indexCount, 0, 0);
	context->GSSetShader(nullptr, nullptr, 0);

	UINT vertCoralStride = 1;
	context->RSSetState(m_noCullRasterState.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->IASetVertexBuffers(0, 1, m_vertexCoralBuffer.GetAddressOf(), &vertCoralStride, &offset);
	context->IASetInputLayout(m_coralVertexInputLayout.Get());
	context->VSSetShader(m_vCoralVertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_vCoralPixelShader.Get(), nullptr, 0);
	context->Draw(misc.x, 0);
	context->OMSetDepthStencilState(nullptr, 0);

}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SDFVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SDFPixelShader.cso");
	auto loadVertexCoralVS = DX::ReadDataAsync(L"CoralVVertexShader.cso");
	auto loadVertexCoralPS = DX::ReadDataAsync(L"CoralVPixelShader.cso");
	auto loadGeomCoralVS = DX::ReadDataAsync(L"CoralGVertexShader.cso");
	auto loadGeomCoralPS = DX::ReadDataAsync(L"CoralGPixelShader.cso");
	auto loadGeomCoralGS = DX::ReadDataAsync(L"CoralGGeomShader.cso");
	auto loadFishVS = DX::ReadDataAsync(L"FishVertexShader.cso");
	auto loadFishPS = DX::ReadDataAsync(L"FishPixelShader.cso");
	auto loadFishGS = DX::ReadDataAsync(L"FishGeometryShader.cso");

	auto* device = m_deviceResources->GetD3DDevice();

	auto createFishVS = loadFishVS.then([this, device](const std::vector<byte>& data) {
		DX::ThrowIfFailed(device->CreateVertexShader(&data[0], data.size(), nullptr, m_fishVertexShader.GetAddressOf()));
	});

	auto createFishPS = loadFishPS.then([this, device](const std::vector<byte>& data) {
		DX::ThrowIfFailed(device->CreatePixelShader(&data[0], data.size(), nullptr, m_fishPixelShader.GetAddressOf()));
	});

	auto createFishGS = loadFishGS.then([this, device](const std::vector<byte>& data) {
		DX::ThrowIfFailed(device->CreateGeometryShader(&data[0], data.size(), nullptr, m_fishGeomShader.GetAddressOf()));
	});

	auto createGVertexCoralTask = loadGeomCoralVS.then([this, device](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(device->CreateVertexShader(&fileData[0], fileData.size(), nullptr, m_gCoralVertexShader.GetAddressOf()));

		static const D3D11_INPUT_ELEMENT_DESC geomVertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		DX::ThrowIfFailed(device->CreateInputLayout(geomVertexDesc, ARRAYSIZE(geomVertexDesc), &fileData[0], fileData.size(), m_coralGeomInputLayout.GetAddressOf()));
	});

	auto createGPixelCoralTask = loadGeomCoralPS.then([this, device](const std::vector<byte>& data) {
		DX::ThrowIfFailed(device->CreatePixelShader(&data[0], data.size(), nullptr, m_gCoralPixelShader.GetAddressOf()));
	});

	auto createGGeomCoralTask = loadGeomCoralGS.then([this, device](const std::vector<byte>& data) {
		DX::ThrowIfFailed(device->CreateGeometryShader(&data[0], data.size(), nullptr, m_gCoralGeomShader.GetAddressOf()));
	});

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_sdfVertexShader.GetAddressOf()
				)
			);

		static const D3D11_INPUT_ELEMENT_DESC SDFInputLayout [] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				SDFInputLayout,
				ARRAYSIZE(SDFInputLayout),
				&fileData[0],
				fileData.size(),
				m_sdfInputLayout.GetAddressOf()
				)
			);
	});

	auto createVertexCoralVS = loadVertexCoralVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, m_vCoralVertexShader.GetAddressOf())
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexCoralLayout[] = {
			{"POSITION", 0, DXGI_FORMAT_R8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,0}
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexCoralLayout, ARRAYSIZE(vertexCoralLayout), &fileData[0], fileData.size(), m_coralVertexInputLayout.GetAddressOf())
		);
	});

	auto createVertexCoralPS = loadVertexCoralPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, m_vCoralPixelShader.GetAddressOf())
		);
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_sdfPixelShader.GetAddressOf()
				)
			);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(MyConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				m_constantBuffer.GetAddressOf()
				)
			);

		constantBufferDesc = CD3D11_BUFFER_DESC(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				m_modelBuffer.GetAddressOf()
			)
		);

		constantBufferDesc = CD3D11_BUFFER_DESC(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				m_lightBuffer.GetAddressOf()
			)
		);
	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this] () {
		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPosition canvasVertices[] = 
		{
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f) },
			{ XMFLOAT3(-0.5f,  0.5f,  0.5f) },
			{ XMFLOAT3(0.5f, 0.5f, 0.5f)},
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f) },
			{ XMFLOAT3(0.5f, 0.5f, 0.5f)},
			{ XMFLOAT3(0.5f, -0.5f, 0.5f)}
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = canvasVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(canvasVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				m_vertexBuffer.GetAddressOf()
				)
			);

		//Create positions for shoal of fish
		static std::vector<VertexPosition> fishPositions;
		const float r = 0.04f;
		for (int i = 0; i < NUM_OF_FISH; ++i) {
			VertexPosition p;
			float theta = 2 * XM_PI * i / NUM_OF_FISH; // angle around the ring
			p.pos = XMFLOAT3(r * cos(theta) - 0.03, 0.14f + sin(i*3.0)*0.01, r * sin(theta)); // x and z coordinates
			fishPositions.push_back(p);
		}

		D3D11_SUBRESOURCE_DATA fishBufferData = { 0 };
		fishBufferData.pSysMem = fishPositions.data();
		fishBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC fishBufferDesc(sizeof(VertexPosition) * fishPositions.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&fishBufferDesc,
				&fishBufferData,
				m_fishBuffer.GetAddressOf()
			)
		);

		//Create icosphere for geom coral
		static std::vector<VertexPosNormTex> vertices;
		const float radius = 0.25f;
		const float stackCount = 18;
		const float sectorCount = 36;

		float x, y, z, xy;
		float nx, ny, nz, lengthInv = 1.0f / radius;
		float s, t;
		float sectorStep = 2 * 3.14 / sectorCount;
		float stackStep = 3.14 / stackCount;
		float sectorAngle, stackAngle;

		for (int i = 0; i <= stackCount; ++i) {
			stackAngle = 3.14 / 2 - i * stackStep;
			xy = radius * cosf(stackAngle);
			z = radius * sinf(stackAngle);

			for (int j = 0; j <= sectorCount; ++j) {
				sectorAngle = j * sectorStep;

				x = xy * cosf(sectorAngle);
				y = xy * sinf(sectorAngle);

				nx = x * lengthInv;
				ny = y * lengthInv;
				nz = z * lengthInv;

				s = static_cast<float>(j) / sectorCount;
				t = static_cast<float>(i) / stackCount;
				VertexPosNormTex vpn{};
				vpn.pos = XMFLOAT3(x, y, z);
				vpn.normal = XMFLOAT3(nx, ny, nz);
				vpn.texcoord = XMFLOAT2(s, t);
				vertices.push_back(vpn);
			}
		}

		D3D11_SUBRESOURCE_DATA vbd = { 0 };
		vbd.pSysMem = vertices.data();
		vbd.SysMemPitch = 0;
		vbd.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vbdesc(vertices.size() * sizeof(VertexPosNormTex), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vbdesc,
				&vbd,
				&m_geomCoralBuffer
			)
		);

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.
		static std::vector<unsigned short> indices;

		int k1, k2;
		for (int i = 0; i < stackCount; ++i) {

			k1 = i * (sectorCount + 1);
			k2 = k1 + sectorCount + 1;
			for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
				if (i != 0) {
					indices.push_back(k1);
					indices.push_back(k2);
					indices.push_back(k1 + 1);
				}

				if (i != (stackCount - 1)) {
					indices.push_back(k1 + 1);
					indices.push_back(k2);
					indices.push_back(k2 + 1);
				}
			}
		}

		m_indexCount = indices.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = indices.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short) * indices.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_geomCoralIndexBuffer
			)
		);
		
	});

	auto createCoralVertexTask = (createVertexCoralVS && createVertexCoralPS).then([this]() {
		std::vector<char> nullVertices(static_cast<int>(misc.x), 0);
		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = nullVertices.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(nullVertices.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, m_vertexCoralBuffer.GetAddressOf())
		);
	});

	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});

	auto loadSeafloorTexture = DX::ReadDataAsync(L"seafloor.dds");
	auto createSeafloorTexture = loadSeafloorTexture.then([this](std::vector<byte>& fileData) {
		CreateDDSTextureFromMemory(m_deviceResources->GetD3DDevice(),
			fileData.data(), fileData.size(),
			m_seafloorTexture.GetAddressOf(), m_seafloorSRV.GetAddressOf());

		D3D11_SAMPLER_DESC sd{};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
		sd.MipLODBias = 0;
		sd.MaxAnisotropy = 1;
		sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sd.MinLOD = -FLT_MAX;
		sd.MaxLOD = FLT_MAX;

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&sd, m_sampler.GetAddressOf()));
	});

	auto loadCoralTexture = DX::ReadDataAsync(L"coral.dds");
	auto createCoralTexture = loadCoralTexture.then([this](std::vector<byte>& fileData) {
		CreateDDSTextureFromMemory(m_deviceResources->GetD3DDevice(), fileData.data(), fileData.size(), m_coralTexture.GetAddressOf(), m_coralSRV.GetAddressOf());
	});

	auto loadVertCoralTexture = DX::ReadDataAsync(L"vertexCoralTexture.dds");
	auto createVertCoralTexture = loadVertCoralTexture.then([this](std::vector<byte>& data) {
		CreateDDSTextureFromMemory(m_deviceResources->GetD3DDevice(), data.data(), data.size(), m_vertCoralTexture.GetAddressOf(), m_vertCoralSRV.GetAddressOf());
	});

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	m_deviceResources->GetD3DDevice()->CreateBlendState(&blendDesc, m_blendStateAlpha.GetAddressOf());

	D3D11_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc = CD3D11_DEPTH_STENCIL_DESC();
	depthDesc.DepthEnable = false;
	m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&depthDesc, m_noDepthState.GetAddressOf());

	D3D11_RASTERIZER_DESC rd = CD3D11_RASTERIZER_DESC();
	rd.CullMode = D3D11_CULL_NONE;
	rd.FillMode = D3D11_FILL_SOLID;
	HRESULT hr = m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rd, m_noCullRasterState.GetAddressOf());
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_sdfVertexShader.Reset();
	m_sdfInputLayout.Reset();
	m_sdfPixelShader.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_modelBuffer.Reset();
	m_sampler.Reset();
	m_seafloorSRV.Reset();
	m_seafloorTexture.Reset();
}