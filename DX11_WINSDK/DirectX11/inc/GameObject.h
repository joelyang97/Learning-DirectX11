#pragma once
#include "Effects.h"
#include "Geometry.h"
#include "Transform.h"

class GameObject {
public:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	GameObject();

	Transform& GetTransform();
	const Transform& GetTransform() const;

	template<class VertexType, class IndexType>
	void SetBuffer(ID3D11Device* device, const Geometry::MeshData<VertexType, IndexType>& meshData);
	void SetTexture(ID3D11ShaderResourceView* texture);
	void SetMaterial(const Material& material);

	void Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect);

	void SetDebugObjectName(const std::string& name);

private:
	Transform m_Transfrom;
	Material m_Material;
	ComPtr<ID3D11ShaderResourceView> m_pTexture;
	ComPtr<ID3D11Buffer> m_pVertexBuffer;
	ComPtr<ID3D11Buffer> m_pIndexBuffer;
	UINT m_VertexStride;
	UINT m_IndexCount;

};

template<class VertexType, class IndexType>
inline void GameObject::SetBuffer(ID3D11Device* device, const Geometry::MeshData<VertexType, IndexType>& meshData) {
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();

	if (device == nullptr)
		return;

	m_VertexStride = sizeof(VertexType);
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = m_VertexStride * (UINT)meshData.vertexVec.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	device->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf());

	m_IndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = m_IndexCount * sizeof(IndexType);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	InitData.pSysMem = meshData.indexVec.data();
	device->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf());
}