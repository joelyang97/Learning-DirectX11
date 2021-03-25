#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"

using namespace DirectX;




GameApp::GameApp(HINSTANCE hInstance) : D3DApp(hInstance), m_CameraMode(CameraMode::ThirdPerson) {

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

	if (m_pCamera != nullptr) {
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
	}
}

void GameApp::UpdateScene(float dt) {
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	auto cam3rt = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

	if (m_CameraMode == CameraMode::Free || m_CameraMode == CameraMode::FirstPerson) {
		if (keyState.IsKeyDown(Keyboard::W)) {
			if (m_CameraMode == CameraMode::FirstPerson)
				cam1st->Walk(dt * 6.0f);
			else
				cam1st->MoveForward(dt * 6.0f);
		}
		if (keyState.IsKeyDown(Keyboard::S)) {
			if (m_CameraMode == CameraMode::FirstPerson)
				cam1st->Walk(dt * -6.0f);
			else
				cam1st->MoveForward(dt * -6.0f);
		}
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -6.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 6.0f);

		XMFLOAT3 adjustedPos;
		XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorSet(-8.9f, 0.0f, -8.9f, 0.0f), XMVectorReplicate(8.9f)));

		if (m_CameraMode == CameraMode::FirstPerson) {
			m_WoodCrate.GetTransform().SetPosition(adjustedPos);
		}

		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);
	}
	else if (m_CameraMode == CameraMode::ThirdPerson) {
		cam3rt->SetTarget(m_WoodCrate.GetTransform().GetPosition());

		if (mouseState.positionMode == Mouse::MODE_RELATIVE) {
			cam3rt->RotateX(mouseState.y * dt * 1.25f);
			cam3rt->RotateY(mouseState.x * dt * 1.25f);
			cam3rt->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
		}
	}

	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

	m_pMouse->ResetScrollWheelValue();

	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1) && m_CameraMode != CameraMode::FirstPerson) {
		if (!cam1st) {
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam1st;
		}

		XMFLOAT3 pos = m_BoltAnim.GetTransform().GetPosition();
		XMFLOAT3 target = (!pos.x && !pos.z ? XMFLOAT3{ 0.0f, 0.0f, 1.0f } : XMFLOAT3{});
		cam1st->LookAt(pos, target, XMFLOAT3(0.0f, 1.0f, 0.0f));

		m_CameraMode = CameraMode::FirstPerson;
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2) && m_CameraMode != CameraMode::ThirdPerson) {
		if (!cam3rt) {
			cam3rt.reset(new ThirdPersonCamera);
			cam3rt->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam3rt;
		}

		XMFLOAT3 target = m_BoltAnim.GetTransform().GetPosition();
		cam3rt->SetTarget(target);
		cam3rt->SetDistance(5.0f);
		cam3rt->SetDistanceMinMax(2.0f, 14.0f);
		cam3rt->SetRotationX(XM_PIDIV2);

		m_CameraMode = CameraMode::ThirdPerson;
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2) && m_CameraMode != CameraMode::Free) 
	{
		if (!cam1st) {
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam1st;
		}

		XMFLOAT3 pos = m_BoltAnim.GetTransform().GetPosition();
		XMFLOAT3 look{ 0.0f, 0.0f, 1.0f };
		XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
		pos.y += 3;
		cam1st->LookTo(pos, look, up);

		m_CameraMode = CameraMode::Free;
	}

	if (keyState.IsKeyDown(Keyboard::Escape))
		SendMessage(MainHnd(), WM_DESTROY, 0, 0);

	static int currBoltFrame = 0;
	static float frameTime = 0.0f;
	m_BoltAnim.SetTexture(mBoltSRVs[currBoltFrame].Get());
	if (frameTime > 1.0f / 60) {
		currBoltFrame = (currBoltFrame + 1) % 60;
		frameTime -= 1.0f / 60;
	}
	frameTime += dt;
}

void GameApp::DrawScene() {
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	m_BasicEffect.SetWriteStencilOnly(m_pd3dImmediateContext.Get(), 1);
	m_Mirror.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);


	m_BasicEffect.SetReflectionState(true);
	m_BasicEffect.SetRenderDefaultWithStencil(m_pd3dImmediateContext.Get(), 1);

	m_Walls[2].Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_Walls[3].Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_Walls[4].Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_Floor.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_WoodCrate.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);


	m_WoodCrate.SetMaterial(m_ShadowMat);
	m_BasicEffect.SetShadowState(true);
	m_BasicEffect.SetRenderNoDoubleBlend(m_pd3dImmediateContext.Get(), 1);

	m_WoodCrate.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	m_BasicEffect.SetShadowState(false);
	m_WoodCrate.SetMaterial(m_WoodCrateMat);

	m_BasicEffect.SetDrawBoltAnimNoDepthWriteWithStencil(m_pd3dImmediateContext.Get(), 1);
	m_BoltAnim.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	m_BasicEffect.SetReflectionState(false);
	m_BasicEffect.SetRenderAlphaBlendWithStencil(m_pd3dImmediateContext.Get(), 1);

	m_Mirror.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
	for (auto& wall : m_Walls)
		wall.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_Floor.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_WoodCrate.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	m_WoodCrate.SetMaterial(m_ShadowMat);
	m_BasicEffect.SetShadowState(true);
	m_BasicEffect.SetRenderNoDoubleBlend(m_pd3dImmediateContext.Get(), 0);

	m_WoodCrate.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	m_BasicEffect.SetShadowState(false);
	m_WoodCrate.SetMaterial(m_WoodCrateMat);

	m_BasicEffect.SetDrawBoltAnimNoDepthWrite(m_pd3dImmediateContext.Get());
	m_BoltAnim.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	if (m_pd2dRenderTarget != nullptr) {
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"切换摄像机模式: 1-第一人称 2-第三人称 3-自由视角\n"
			L"W/S/A/D 前进/后退/左平移/右平移 (第三人称无效)  Esc退出\n"
			L"鼠标移动控制视野 滚轮控制第三人称观察距离\n"
			L"当前模式: ";
		if (m_CameraMode == CameraMode::FirstPerson)
			text += L"第一人称(控制箱子移动)";
		else if (m_CameraMode == CameraMode::ThirdPerson)
			text += L"第三人称";
		else
			text += L"自由视角";
		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}
	HR(m_pSwapChain->Present(0, 0));
}


bool GameApp::InitResource() {


	ComPtr<ID3D11ShaderResourceView> texture;
	Material material{};
	material.ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	material.specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 16.0f);

	m_WoodCrateMat = material;
	m_ShadowMat.ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_ShadowMat.diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	m_ShadowMat.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

	mBoltSRVs.assign(60, nullptr);
	wchar_t wstr[50];
	for (int i = 1; i <= 60; ++i) {
		wsprintf(wstr, L"..\\Texture\\BoltAnim\\Bolt%03d.bmp", i);
		HR(CreateWICTextureFromFile(m_pd3dDevice.Get(), wstr, nullptr, mBoltSRVs[static_cast<size_t>(i) - 1].GetAddressOf()));
	}
	m_BoltAnim.SetBuffer(m_pd3dDevice.Get(), Geometry::CreateCylinderNoCap(4.0f, 4.0f));
	m_BoltAnim.GetTransform().SetPosition(0.0f, 2.01f, 0.0f);
	m_BoltAnim.SetMaterial(material);

	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\WoodCrate.dds", nullptr, texture.GetAddressOf()));
	m_WoodCrate.SetBuffer(m_pd3dDevice.Get(), Geometry::CreateBox());
	m_WoodCrate.GetTransform().SetPosition(0.0f, 0.01f, 0.0f);
	m_WoodCrate.SetTexture(texture.Get());
	m_WoodCrate.SetMaterial(material);

	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\floor.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	m_Floor.SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
	m_Floor.SetTexture(texture.Get());
	m_Floor.SetMaterial(material);
	m_Floor.GetTransform().SetPosition(0.0f, -1.0f, 0.0f);

	m_Walls.resize(5);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\brick.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	for (int i = 0; i < 5; ++i) {

		m_Walls[i].SetMaterial(material);
		m_Walls[i].SetTexture(texture.Get());
	}
	m_Walls[0].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
	m_Walls[1].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
	m_Walls[2].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	m_Walls[3].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	m_Walls[4].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));

	m_Walls[0].GetTransform().SetRotation(-XM_PIDIV2, 0.0f, 0.0f);
	m_Walls[0].GetTransform().SetPosition(-7.0f, 3.0f, 10.0f);
	m_Walls[1].GetTransform().SetRotation(-XM_PIDIV2, 0.0f, 0.0f);
	m_Walls[1].GetTransform().SetPosition(7.0f, 3.0f, 10.0f);
	m_Walls[2].GetTransform().SetRotation(-XM_PIDIV2, XM_PIDIV2, 0.0f);
	m_Walls[2].GetTransform().SetPosition(10.0f, 3.0f, 0.0f);
	m_Walls[3].GetTransform().SetRotation(-XM_PIDIV2, XM_PI, 0.0f);
	m_Walls[3].GetTransform().SetPosition(0.0f, 3.0f, -10.0f);
	m_Walls[4].GetTransform().SetRotation(-XM_PIDIV2, -XM_PIDIV2, 0.0f);
	m_Walls[4].GetTransform().SetPosition(-10.0f, 3.0f, 0.0f);


	// 初始化镜面
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\ice.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	m_Mirror.SetBuffer(m_pd3dDevice.Get(),
		Geometry::CreatePlane(XMFLOAT2(8.0f, 8.0f), XMFLOAT2(1.0f, 1.0f)));
	m_Mirror.GetTransform().SetRotation(-XM_PIDIV2, 0.0f, 0.0f);
	m_Mirror.GetTransform().SetPosition(0.0f, 3.0f, 10.0f);
	m_Mirror.SetTexture(texture.Get());
	m_Mirror.SetMaterial(material);


	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetDistance(5.0f);
	camera->SetTarget(XMFLOAT3(0.0f, 0.5f, 0.0f));
	camera->SetDistanceMinMax(2.0f, 14.0f);
	camera->SetRotationX(XM_PIDIV2);
	

	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

	m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);

	m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());

	m_BasicEffect.SetReflectionMatrix(XMMatrixReflect(XMVectorSet(0.0f, 0.0f, -1.0f, 10.0f)));
	m_BasicEffect.SetShadowMatrix(XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.99f), XMVectorSet(0.0f, 10.0f, -10.0f, 1.0f)));
	m_BasicEffect.SetRefShadowMatrix(XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.99f), XMVectorSet(0.0f, 10.0f, 30.0f, 1.0f)));



	// 环境光
	DirectionalLight dirLight;
	dirLight.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_BasicEffect.SetDirLight(0, dirLight);
	// 灯光
	PointLight pointLight;
	pointLight.position = XMFLOAT3(0.0f, 10.0f, -10.0f);
	pointLight.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	pointLight.diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	pointLight.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	pointLight.att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	pointLight.range = 25.0f;
	m_BasicEffect.SetPointLight(0, pointLight);

	m_BoltAnim.SetDebugObjectName("BoltAnim");
	
	m_Floor.SetDebugObjectName("Floor");
	m_Mirror.SetDebugObjectName("Mirror");
	m_Walls[0].SetDebugObjectName("Walls[0]");
	m_Walls[1].SetDebugObjectName("Walls[1]");
	m_Walls[2].SetDebugObjectName("Walls[2]");
	m_Walls[3].SetDebugObjectName("Walls[3]");
	m_Walls[4].SetDebugObjectName("Walls[4]");
	m_WoodCrate.SetDebugObjectName("WoodCrate");
	return true;
}