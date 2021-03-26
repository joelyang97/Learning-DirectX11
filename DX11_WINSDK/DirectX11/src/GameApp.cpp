#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"

using namespace DirectX;




GameApp::GameApp(HINSTANCE hInstance) : D3DApp(hInstance), m_ShowMode(Mode::SplitedTriangle), m_VertexCount() {

}

GameApp::~GameApp() {

}

bool GameApp::Init() {
	m_pMouse = std::make_unique<DirectX::Mouse>();
	m_pKeyboard = std::make_unique<DirectX::Keyboard>();

	if (!D3DApp::Init())
		return false;

	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

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
		OutputDebugStringW(L"\n警告：Direct2D与Direct3D互操作性功能受限，你将无法看到文本信息。现提供下述选方法：\n"
			L"1. 对于Win7系统，需要更新至Win7 SP1，并安KB2670838补丁以支持Direct2D显示。\n"
			L"2. 行完成Direct3D 10.1与Direct2D的交互。情参阅"
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

	m_BasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));
}

void GameApp::UpdateScene(float dt) {
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);


	if (m_ShowMode == Mode::SplitedTriangle) {
		m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	}
	else {
		static float phi = 0.0f, theta = 0.0f;
		phi += 0.2f * dt, theta += 0.3f * dt;
		m_BasicEffect.SetWorldMatrix(XMMatrixRotationX(phi) * XMMatrixRotationY(theta));
	}

	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1)) {
		m_ShowMode = Mode::SplitedTriangle;
		ResetTriangle();
		UINT stride = sizeof(VertexPosColor);
		UINT offset = 0;
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
		m_BasicEffect.SetRenderSplitedTriangle(m_pd3dImmediateContext.Get());
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2)) {
		m_ShowMode = Mode::CylinderNoCap;
		ResetRoundWire();
		UINT stride = sizeof(VertexPosNormalColor);
		UINT offset = 0;
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
		m_BasicEffect.SetRenderCylinderNoCap(m_pd3dImmediateContext.Get());
	}

	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Q)) {
		if (m_ShowMode == Mode::CylinderNoCap) {
			m_ShowMode = Mode::CylinderNoCapWithNormal;
		}
		else if (m_ShowMode == Mode::CylinderNoCapWithNormal) {
			m_ShowMode = Mode::CylinderNoCap;
		}
	}
}

void GameApp::DrawScene() {
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
	m_pd3dImmediateContext->Draw(m_VertexCount, 0);
	if (m_ShowMode == Mode::CylinderNoCapWithNormal) {
		m_BasicEffect.SetRenderNormal(m_pd3dImmediateContext.Get());
		m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
		m_pd3dImmediateContext->Draw(m_VertexCount, 0);
		m_BasicEffect.SetRenderCylinderNoCap(m_pd3dImmediateContext.Get());
	}



	if (m_pd2dRenderTarget != nullptr) {
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"切换类型：1-分裂的三角形 2-圆线构造柱面\n"
			L"当前模式: ";
		if (m_ShowMode == Mode::SplitedTriangle)
			text += L"分裂的三角形";
		else if (m_ShowMode == Mode::CylinderNoCap)
			text += L"圆线构造柱面(Q-显示圆线的法向量)";
		else
			text += L"圆线构造柱面(Q-隐藏圆线的法向量)";
		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}
	HR(m_pSwapChain->Present(0, 0));
}


bool GameApp::InitResource() {

	ResetTriangle();

	// 方向光
	DirectionalLight dirLight;
	dirLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	m_BasicEffect.SetDirLight(0, dirLight);
	// 材质
	Material material{};
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
	m_BasicEffect.SetMaterial(material);
	// 摄像机位置
	m_BasicEffect.SetEyePos(XMFLOAT3(0.0f, 0.0f, -5.0f));
	// 矩阵
	m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	m_BasicEffect.SetViewMatrix(XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f),
		XMVectorZero(),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
	m_BasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));
	// 圆柱高度
	m_BasicEffect.SetCylinderHeight(2.0f);

	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexPosColor);		// 跨越字节数
	UINT offset = 0;							// 起始偏移量
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	// 设置默认渲染状态
	m_BasicEffect.SetRenderSplitedTriangle(m_pd3dImmediateContext.Get());

	return true;
}

void GameApp::ResetTriangle() {
	VertexPosColor vertices[] = {
		{XMFLOAT3(-1.0f * 3, -0.886f * 3, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
		{XMFLOAT3(0.0f * 3, 0.886f * 3, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		{XMFLOAT3(1.0f * 3, -0.886f * 3, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)}
	};

	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf()));
	m_VertexCount = 3;

	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "TriangleVertexBuffer");
}

void GameApp::ResetRoundWire() {
	VertexPosNormalColor vertices[41];
	for (int i = 0; i < 40; ++i) {
		vertices[i].pos = XMFLOAT3(cosf(XM_PI / 20 * i), -1.0f, -sinf(XM_PI / 20 * i));
		vertices[i].normal = XMFLOAT3(cosf(XM_PI / 20 * i), 0.0f, -sinf(XM_PI / 20 * i));
		vertices[i].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	vertices[40] = vertices[0];

	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf()));

	m_VertexCount = 41;

	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "CylinderVertexBuffer");
}