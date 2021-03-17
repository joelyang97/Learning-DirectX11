#pragma once

#include "d3dApp.h"
#include "LightHelper.h"
#include "Geometry.h"
#include "Camera.h"

class GameApp : public D3DApp {
public:
	struct CBChangesEveryDrawing {
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
	};

	struct CBChangesEveryFrame {
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT4 eyePos;
	};

	struct CBChangesOnResize {
		DirectX::XMMATRIX proj;
	};

	struct CBChangesRarely {
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		Material material;
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;
	};

	class GameObject {
	public:
		GameObject();

		Transform& GetTransform();
		const Transform& GetTransform() const;

		template<class VertexType, class IndexType>
		void SetBuffer(ID3D11Device* device, const Geometry::MeshData<VertexType, IndexType>& meshData);
		void SetTexture(ID3D11ShaderResourceView* texture);

		void Draw(ID3D11DeviceContext* deviceContext);
		void SetDebugObjectName(const std::string& name);

	private:
		Transform m_Transform;
		ComPtr<ID3D11ShaderResourceView> m_pTexture;
		ComPtr<ID3D11Buffer> m_pVertexBuffer;
		ComPtr<ID3D11Buffer> m_pIndexBuffer;
		UINT m_VertexStride;
		UINT m_IndexCount;
	};

	enum class CameraMode{FirstPerson, ThirdPerson, Free};

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


private:
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;
	ComPtr<IDWriteFont> m_pFont;
	ComPtr<IDWriteTextFormat> m_pTextFormat;

	ComPtr<ID3D11InputLayout> m_pVertexLayout2D;
	ComPtr<ID3D11InputLayout> m_pVertexLayout3D;
	ComPtr<ID3D11Buffer> m_pConstantBuffers[4];

	GameObject m_WoodCrate;
	GameObject m_Floor;
	std::vector<GameObject> m_Walls;

	ComPtr<ID3D11VertexShader> m_pVertexShader3D;
	ComPtr<ID3D11PixelShader> m_pPixelShader3D;
	ComPtr<ID3D11VertexShader> m_pVertexShader2D;
	ComPtr<ID3D11PixelShader> m_pPixelShader2D;

	CBChangesEveryFrame m_CBFrame;
	CBChangesOnResize m_CBOnResize;
	CBChangesRarely m_CBRarely;

	ComPtr<ID3D11SamplerState> m_pSamplerState;

	std::shared_ptr<Camera> m_pCamera;
	CameraMode m_CameraMode;

};