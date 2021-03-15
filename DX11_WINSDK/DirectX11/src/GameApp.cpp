#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"

using namespace DirectX;




GameApp::GameApp(HINSTANCE hInstance) : D3DApp(hInstance), m_IndexCount(), m_VSConstantBuffer(), m_PSConstantBuffer(), m_DirLight(), m_PointLight(), m_SpotLight(), m_IsWireframeMode(false) {

}

GameApp::~GameApp() {

}

bool GameApp::Init() {
	m_pMouse = std::make_unique<DirectX::Mouse>();
	m_pKeyboard = std::make_unique<DirectX::Keyboard>();

	if (!D3DApp::Init())
		return false;

	if (!InitEffect())
		return false;

	if (!InitResource())
		return false;

	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);
	return true;
}

void GameApp::OnResize() {
	D3DApp::OnResize();
}

void GameApp::UpdateScene(float dt) {

	static float phi = 0.0f, theta = 0.0f;
	phi += 0.0001f, theta += 0.00015f;

	XMMATRIX W = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
	m_VSConstantBuffer.world = XMMatrixTranspose(W);
	m_VSConstantBuffer.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));

	Mouse::State mouseState = m_pMouse->GetState();
	m_MouseTracker.Update(mouseState);
	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1)) {
		m_PSConstantBuffer.dirLight = m_DirLight;
		m_PSConstantBuffer.pointLight = PointLight();
		m_PSConstantBuffer.spotLight = SpotLight();
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_PSConstantBuffer.dirLight = DirectionalLight();
		m_PSConstantBuffer.pointLight = m_PointLight;
		m_PSConstantBuffer.spotLight = SpotLight();
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		m_PSConstantBuffer.dirLight = DirectionalLight();
		m_PSConstantBuffer.pointLight = PointLight();
		m_PSConstantBuffer.spotLight = m_SpotLight;
	}

	// 键盘切换模型类型
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
	{
		auto meshData = Geometry::CreateBox<VertexPosNormalColor>();
		ResetMesh(meshData);
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::W))
	{
		auto meshData = Geometry::CreateSphere<VertexPosNormalColor>();
		ResetMesh(meshData);
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::E))
	{
		auto meshData = Geometry::CreateCylinder<VertexPosNormalColor>();
		ResetMesh(meshData);
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::R))
	{
		auto meshData = Geometry::CreateCone<VertexPosNormalColor>();
		ResetMesh(meshData);
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::S))
	{
		m_IsWireframeMode = !m_IsWireframeMode;
		m_pd3dImmediateContext->RSSetState(m_IsWireframeMode ? m_pRSWireframe.Get() : nullptr);
	}

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(VSConstantBuffer), &m_VSConstantBuffer, sizeof(VSConstantBuffer));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[0].Get(), 0);

	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);



}

void GameApp::DrawScene() {
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);
	HR(m_pSwapChain->Present(0, 0));
}

bool GameApp::InitEffect() {
	ComPtr<ID3DBlob> blob;

	HR(CreateShaderFromFile(L"HLSL\\Light_VS.cso", L"HLSL\\Light_VS.hlsl", "MainVS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader.GetAddressOf()));
	HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout), blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Light_PS.cso", L"HLSL\\Light_PS.hlsl", "MainPS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf()));

	return true;
}

bool GameApp::InitResource() {
	// 设置三角形顶点
	auto meshData = Geometry::CreateBox<VertexPosNormalColor>();
	ResetMesh(meshData);

	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.ByteWidth = sizeof(VSConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = sizeof(PSConstantBuffer);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));

	m_DirLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_DirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_DirLight.direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);

	m_PointLight.position = XMFLOAT3(0.0f, 0.0f, -10.0f);
	m_PointLight.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_PointLight.diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_PointLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_PointLight.att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	m_PointLight.range = 25.0f;

	m_SpotLight.position = XMFLOAT3(0.0f, 0.0f, -5.0f);
	m_SpotLight.direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_SpotLight.ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_SpotLight.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_SpotLight.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_SpotLight.att = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_SpotLight.spot = 12.0f;
	m_SpotLight.range = 10000.0f;

	m_VSConstantBuffer.world = XMMatrixIdentity();
	m_VSConstantBuffer.view = XMMatrixTranspose(XMMatrixLookAtLH(
	XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f), 
	XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), 
	XMVectorSet(0.0f, 1.0f, 0.0f, 0.0)
	));
	m_VSConstantBuffer.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f));
	m_VSConstantBuffer.worldInvTranspose = XMMatrixIdentity();
	
	m_PSConstantBuffer.material.ambient = XMFLOAT4(0.5, 0.5f, 0.5, 1.0f);
	m_PSConstantBuffer.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_PSConstantBuffer.material.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
	m_PSConstantBuffer.dirLight = m_DirLight;
	m_PSConstantBuffer.eyePos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);

	D3D11_MAPPED_SUBRESOURCE mappdedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappdedData));
	memcpy_s(mappdedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);

	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthClipEnable = true;
	HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRSWireframe.GetAddressOf()));




	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout.Get());

	
	m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
	m_pd3dImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	
	m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	m_pd3dImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	D3D11SetDebugObjectName(m_pVertexLayout.Get(), "VertexPosNormalColorLayout");
	D3D11SetDebugObjectName(m_pConstantBuffers[0].Get(), "VSConstantBuffer");
	D3D11SetDebugObjectName(m_pConstantBuffers[1].Get(), "PSConstantBuffer");
	D3D11SetDebugObjectName(m_pVertexShader.Get(), "Light_VS");
	D3D11SetDebugObjectName(m_pPixelShader.Get(), "Light_PS");

	return true;
}

bool GameApp::ResetMesh(const Geometry::MeshData<VertexPosNormalColor>& meshData) {
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();

	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(VertexPosNormalColor);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));

	UINT stride = sizeof(VertexPosNormalColor);
	UINT offset = 0;

	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

	m_IndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = m_IndexCount * sizeof(DWORD);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	InitData.pSysMem = meshData.indexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "IndexBuffer");
	return true;
}