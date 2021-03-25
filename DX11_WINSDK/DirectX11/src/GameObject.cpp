#include "GameObject.h"
#include "d3dUtil.h"
using namespace DirectX;

GameObject::GameObject() : m_IndexCount(), m_Material(), m_VertexStride() {

}

Transform& GameObject::GetTransform() {
	return m_Transfrom;
}

const Transform& GameObject::GetTransform() const {
	return m_Transfrom;
}

void GameObject::SetTexture(ID3D11ShaderResourceView* texture) {
	m_pTexture = texture;
}

void GameObject::SetMaterial(const Material& material) {
	m_Material = material;
}

void GameObject::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect) {
	UINT strides = m_VertexStride;
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &strides, &offset);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	effect.SetWorldMatrix(m_Transfrom.GetLocalToWorldMatrixXM());
	effect.SetTexture(m_pTexture.Get());
	effect.SetMaterial(m_Material);
	effect.Apply(deviceContext);

	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void GameObject::SetDebugObjectName(const std::string& name) {
#if (defined(DEBUG) || defined(_DEBUG) && (GRAPHICS_DEBUGGER_OBJECT_NAME))
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}