#pragma once

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"

class GameApp : public D3DApp {
public:

	enum class CameraMode{FirstPerson, ThirdPerson, Free};

public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();


private:
	bool InitResource();


private:
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;
	ComPtr<IDWriteFont> m_pFont;
	ComPtr<IDWriteTextFormat> m_pTextFormat;

	GameObject m_WoodCrate;
	GameObject m_Floor;
	std::vector<GameObject> m_Walls;
	GameObject m_Mirror;
	GameObject m_BoltAnim;

	std::vector<ComPtr<ID3D11ShaderResourceView>> mBoltSRVs;

	Material m_ShadowMat;
	Material m_WoodCrateMat;

	BasicEffect m_BasicEffect;

	std::shared_ptr<Camera> m_pCamera;
	CameraMode m_CameraMode;

};