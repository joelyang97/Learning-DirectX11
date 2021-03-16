#pragma once

#include "d3dApp.h"
#include "LightHelper.h"
#include "Geometry.h"
class GameApp : public D3DApp {
public:


	struct VSConstantBuffer
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX worldInvTranspose;

	};

	struct PSConstantBuffer
	{
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		Material material;
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;
		DirectX::XMFLOAT4 eyePos;
	};

	enum class ShowMode { WoodCrate, FireAnim };


public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();


private:
	bool InitEffect();
	bool InitResource();

	template<class VertexType>
	bool ResetMesh(const Geometry::MeshData<VertexType>& meshData);

private:
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;
	ComPtr<IDWriteFont> m_pFont;
	ComPtr<IDWriteTextFormat> m_pTextFormat;

	ComPtr<ID3D11InputLayout> m_pVertexLayout2D;
	ComPtr<ID3D11InputLayout> m_pVertexLayout3D;
	ComPtr<ID3D11Buffer> m_pVertexBuffer;
	ComPtr<ID3D11Buffer> m_pIndexBuffer;
	ComPtr<ID3D11Buffer> m_pConstantBuffers[2];
	UINT m_IndexCount;
	int m_CurrFrame;
	ShowMode m_CurrMode;

	ComPtr<ID3D11ShaderResourceView> m_pWoodCrate;
	std::vector<ComPtr<ID3D11ShaderResourceView>> m_pFireAnims;
	ComPtr<ID3D11SamplerState> m_pSamplerState;

	ComPtr<ID3D11VertexShader> m_pVertexShader3D;
	ComPtr<ID3D11PixelShader> m_pPixelShader3D;
	ComPtr<ID3D11VertexShader> m_pVertexShader2D;
	ComPtr<ID3D11PixelShader> m_pPixelShader2D;
	VSConstantBuffer m_VSConstantBuffer;
	PSConstantBuffer m_PSConstantBuffer;

};