#pragma once

#include "d3dApp.h"

class GameApp : public D3DApp {
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

public:
	struct VertexPosColor {
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
	};

	struct ConstantBuffer {
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
	};
private:
	bool InitEffect();
	bool InitResource();

private:
	ComPtr<ID3D11InputLayout> m_pVertexLayout;
	ComPtr<ID3D11Buffer> m_pVertexBuffer;
	ComPtr<ID3D11Buffer> m_pIndexBuffer;
	ComPtr<ID3D11Buffer> m_pConstantBuffer;


	ComPtr<ID3D11VertexShader> m_pVertexShader;
	ComPtr<ID3D11PixelShader> m_pPixelShader;
	ConstantBuffer m_CBuffer;
};