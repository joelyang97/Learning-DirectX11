#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"

using namespace DirectX;




GameApp::GameApp(HINSTANCE hInstance) : D3DApp(hInstance), m_CameraMode(CameraMode::FirstPerson), m_CBFrame(), m_CBOnResize(), m_CBRarely() {

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
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);

	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

	ComPtr<IDXGISurface> surface;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = m_pd2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, m_pd2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE) {
		OutputDebugStringW(L"\n警告：Direct2D与Direct3D互操作性功能受限，你将无法看到文本信息。现提供下述�?选方法：\n"
			L"1. 对于Win7系统，需要更新至Win7 SP1，并安�?�KB2670838补丁以支持Direct2D显示。\n"
			L"2. �?行完成Direct3D 10.1与Direct2D的交互。�?�情参阅�?"
			L"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			L"3. 使用�?的字体库，比如FreeType。\n\n");
	}
	else if (hr == S_OK) {
		HR(m_pd2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			m_pColorBrush.GetAddressOf()));
		HR(m_pdwriteFactory->CreateTextFormat(L"宋体", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20, L"zh-cn",
			m_pTextFormat.GetAddressOf()));
	}
	else {
		assert(m_pd2dRenderTarget);
	}

	if (m_pCamera != nullptr) {
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_CBOnResize.proj = XMMatrixTranspose(m_pCamera->GetProjXM());

		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[2].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
		memcpy_s(mappedData.pData, sizeof(CBChangesOnResize), &m_CBOnResize, sizeof(CBChangesOnResize));
		m_pd3dImmediateContext->Unmap(m_pConstantBuffers[2].Get(), 0);
	}
}

void GameApp::UpdateScene(float dt) {
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	auto cam3st = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);

	if (mouseState.positionMode == Mouse::MODE_RELATIVE) {
		cam3st->RotateX(mouseState.y * dt * 1.25f);
		cam3st->RotateY(mouseState.x * dt * 1.25f);
		cam3st->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
	}

	m_CBFrame.eyePos = m_pCamera->GetPositionXM();
	m_CBFrame.view = XMMatrixTranspose(m_pCamera->GetViewXM());

	m_pMouse->ResetScrollWheelValue();

	if (keyState.IsKeyDown(Keyboard::Escape))
		SendMessage(MainHnd(), WM_DESTROY, 0, 0);

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesEveryFrame), &m_CBFrame, sizeof(CBChangesEveryFrame));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);

}

void GameApp::DrawScene() {
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_pd3dImmediateContext->RSSetState(nullptr);
	m_pd3dImmediateContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);



	m_Floor.Draw(m_pd3dImmediateContext.Get());
	for (auto& wall : m_Walls)
		wall.Draw(m_pd3dImmediateContext.Get());

	m_pd3dImmediateContext->RSSetState(RenderStates::RSNoCull.Get());
	m_pd3dImmediateContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
	Transform& wireFrameTransform = m_WireFence.GetTransform();
	wireFrameTransform.SetPosition(2.0f, 0.01f, 0.0f);
	m_WireFence.Draw(m_pd3dImmediateContext.Get());
	wireFrameTransform.SetPosition(-2.0f, 0.01f, 0.0f);
	m_WireFence.Draw(m_pd3dImmediateContext.Get());
	m_Water.Draw(m_pd3dImmediateContext.Get());

	if (m_pd2dRenderTarget != nullptr) {
		m_pd2dRenderTarget->BeginDraw();
		std::wstring textStr = L"当前摄像机模式：第三人称视角  Esc退出\n"
			L"鼠标移动控制视野 滚轮控制第三人称观察距离\n"
			L"当前模式: ";
		m_pd2dRenderTarget->DrawTextW(textStr.c_str(), (UINT32)textStr.size(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}
	HR(m_pSwapChain->Present(0, 0));
}

bool GameApp::InitEffect() {
	ComPtr<ID3DBlob> blob;
	
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_2D.cso", L"shader\\Basic_VS_2D.hlsl", "VS_2D", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader2D.GetAddressOf()));
	HR(m_pd3dDevice->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout), blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout2D.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_2D.cso", L"shader\\Basic_PS_2D.hlsl", "PS_2D", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader2D.GetAddressOf()));


	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_3D.cso", L"shader\\Basic_VS_3D.hlsl", "VS_3D", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader3D.GetAddressOf()));
	HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout), blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout3D.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_3D.cso", L"shader\\Basic_PS_3D.hlsl", "PS_3D", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader3D.GetAddressOf()));

	return true;
}

bool GameApp::InitResource() {


	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.ByteWidth = sizeof(CBChangesEveryDrawing);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesEveryFrame);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesOnResize);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[2].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesRarely);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[3].GetAddressOf()));

	ComPtr<ID3D11ShaderResourceView> texture;
	Material material{};
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\WireFence.dds", nullptr, texture.GetAddressOf()));
	m_WireFence.SetBuffer(m_pd3dDevice.Get(), Geometry::CreateBox());
	m_WireFence.SetTexture(texture.Get());
	m_WireFence.SetMaterial(material);

	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\floor.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	m_Floor.SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
	m_Floor.SetTexture(texture.Get());
	m_Floor.GetTransform().SetPosition(0.0f, -1.0f, 0.0f);

	m_Walls.resize(4);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\brick.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	for (int i = 0; i < 4; ++i) {
		m_Walls[i].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 1.5f)));
		m_Walls[i].SetMaterial(material);
		Transform& transform = m_Walls[i].GetTransform();
		transform.SetRotation(-XM_PIDIV2, XM_PIDIV2 * i, 0.0f);
		transform.SetPosition(i % 2 ? -10.0f * (i - 2) : 0.0f, 3.0f, i % 2 == 0 ? -10.0f * (i - 1) : 0.0f);
		m_Walls[i].SetTexture(texture.Get());
	}

	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\water.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	m_Water.SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 20.0f), XMFLOAT2(10.0f, 10.0f)));
	m_Water.SetTexture(texture.Get());
	m_Water.SetMaterial(material);


	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetTarget(XMFLOAT3(0.0f, 0.5f, 0.0f));
	camera->SetDistance(8.0f);
	camera->SetDistanceMinMax(2.0f, 14.0f);
	camera->SetRotationX(XM_PIDIV4);
	


	m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
	m_CBOnResize.proj = XMMatrixTranspose(m_pCamera->GetProjXM());

	// 初始化不会变化的值
	// 环境光
	m_CBRarely.dirLight[0].ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.dirLight[0].diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_CBRarely.dirLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.dirLight[0].direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	// 灯光
	m_CBRarely.pointLight[0].position = XMFLOAT3(0.0f, 15.0f, 0.0f);
	m_CBRarely.pointLight[0].ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.pointLight[0].diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	m_CBRarely.pointLight[0].specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_CBRarely.pointLight[0].att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	m_CBRarely.pointLight[0].range = 25.0f;
	m_CBRarely.numDirLight = 1;
	m_CBRarely.numPointLight = 1;
	m_CBRarely.numSpotLight = 0;



	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[2].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesOnResize), &m_CBOnResize, sizeof(CBChangesOnResize));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[2].Get(), 0);

	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[3].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesRarely), &m_CBRarely, sizeof(CBChangesRarely));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[3].Get(), 0);


	RenderStates::InitAll(m_pd3dDevice.Get());


	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());

	m_pd3dImmediateContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0);
	m_pd3dImmediateContext->PSSetShader(m_pPixelShader3D.Get(), nullptr, 0);

	m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
	m_pd3dImmediateContext->VSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	m_pd3dImmediateContext->VSSetConstantBuffers(2, 1, m_pConstantBuffers[2].GetAddressOf());
	

	m_pd3dImmediateContext->PSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
	m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	m_pd3dImmediateContext->PSSetConstantBuffers(3, 1, m_pConstantBuffers[3].GetAddressOf());

	m_pd3dImmediateContext->RSSetState(RenderStates::RSNoCull.Get());
	m_pd3dImmediateContext->PSSetSamplers(0, 1, RenderStates::SSAnistropicWrap.GetAddressOf());
	m_pd3dImmediateContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xffffffff);


	D3D11SetDebugObjectName(m_pVertexLayout2D.Get(), "VertexPosTexLayout");
	D3D11SetDebugObjectName(m_pVertexLayout3D.Get(), "VertexPosNormalTexLayout");
	D3D11SetDebugObjectName(m_pConstantBuffers[0].Get(), "CBDrawing");
	D3D11SetDebugObjectName(m_pConstantBuffers[1].Get(), "CBFrame");
	D3D11SetDebugObjectName(m_pConstantBuffers[2].Get(), "CBOnResize");
	D3D11SetDebugObjectName(m_pConstantBuffers[3].Get(), "CBRarely");
	D3D11SetDebugObjectName(m_pVertexShader2D.Get(), "Basic_VS_2D");
	D3D11SetDebugObjectName(m_pVertexShader3D.Get(), "Basic_VS_3D");
	D3D11SetDebugObjectName(m_pPixelShader2D.Get(), "Basic_PS_2D");
	D3D11SetDebugObjectName(m_pPixelShader3D.Get(), "Basic_PS_3D");
	m_Floor.SetDebugObjectName("Floor");
	m_Walls[0].SetDebugObjectName("Walls[0]");
	m_Walls[1].SetDebugObjectName("Walls[1]");
	m_Walls[2].SetDebugObjectName("Walls[2]");
	m_Walls[3].SetDebugObjectName("Walls[3]");
	return true;
}

GameApp::GameObject::GameObject() : m_IndexCount(), m_VertexStride(){

}

Transform& GameApp::GameObject::GetTransform() {
	return m_Transform;
}

const Transform& GameApp::GameObject::GetTransform() const {
	return m_Transform;
}

template<class VertexType, class IndexType>
void GameApp::GameObject::SetBuffer(ID3D11Device* device, const Geometry::MeshData<VertexType, IndexType>& meshData) {
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();

	m_VertexStride = sizeof(VertexType);
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * m_VertexStride;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	HR(device->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));

	m_IndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = m_IndexCount * sizeof(IndexType);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	InitData.pSysMem = meshData.indexVec.data();
	HR(device->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));


}

void GameApp::GameObject::SetTexture(ID3D11ShaderResourceView* texture) {
	m_pTexture = texture;
}

void GameApp::GameObject::Draw(ID3D11DeviceContext* deviceContext) {
	UINT strides = m_VertexStride;
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &strides, &offset);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	ComPtr<ID3D11Buffer> cBuffer = nullptr;
	deviceContext->VSGetConstantBuffers(0, 1, cBuffer.GetAddressOf());
	CBChangesEveryDrawing cbDrawing;

	XMMATRIX W = m_Transform.GetLocalToWorldMatrixXM();
	cbDrawing.world = XMMatrixTranspose(W);
	cbDrawing.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
	cbDrawing.material = m_Material;
	
	// 更新常量缓冲区
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(deviceContext->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesEveryDrawing), &cbDrawing, sizeof(CBChangesEveryDrawing));
	deviceContext->Unmap(cBuffer.Get(), 0);

	deviceContext->PSSetShaderResources(0, 1, m_pTexture.GetAddressOf());
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);

}

void GameApp::GameObject::SetMaterial(const Material& material)
{
	m_Material = material;
}

void GameApp::GameObject::SetDebugObjectName(const std::string& name) {
#if (defined(DEBUG) || defined(_DEBUG) && (GRAPHICS_DEBUGGER_OBJECT_NAME))
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}