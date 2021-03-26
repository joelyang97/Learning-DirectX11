#pragma once

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"

class GameApp : public D3DApp {
public:


	enum class Mode {SplitedTriangle, CylinderNoCap, CylinderNoCapWithNormal};
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();


private:
	bool InitResource();

	void ResetTriangle();
	void ResetRoundWire();

private:
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;
	ComPtr<IDWriteFont> m_pFont;
	ComPtr<IDWriteTextFormat> m_pTextFormat;

	ComPtr<ID3D11Buffer> m_pVertexBuffer;
	int m_VertexCount;
	Mode m_ShowMode;

	BasicEffect m_BasicEffect;

};