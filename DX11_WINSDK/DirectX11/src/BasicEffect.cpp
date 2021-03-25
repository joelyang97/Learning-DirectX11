#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"
#include "DXTrace.h"
#include "Vertex.h"

using namespace DirectX;

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:
	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
		Material material;
	};

	struct CBDrawingStates
	{
		int isReflection;
		int isShadow;
		DirectX::XMINT2 pad;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT3 eyePos;
		float pad;
	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};

	struct CBChangesRarely
	{
		DirectX::XMMATRIX reflection;
		DirectX::XMMATRIX shadow;
		DirectX::XMMATRIX refShadow;
		DirectionalLight dirLight[BasicEffect::maxLights];
		PointLight pointLight[BasicEffect::maxLights];
		SpotLight spotLight[BasicEffect::maxLights];
	};

public:
	Impl() : m_IsDirty(){}
	~Impl() = default;

public:
	CBufferObject<0, CBChangesEveryDrawing> m_CBDrawing;
	CBufferObject<1, CBDrawingStates> m_CBStates;
	CBufferObject<2, CBChangesEveryFrame> m_CBFrame;
	CBufferObject<3, CBChangesOnResize> m_CBOnResize;
	CBufferObject<4, CBChangesRarely> m_CBRarely;
	BOOL m_IsDirty;
	std::vector<CBufferBase*> m_pCBuffers;

	ComPtr<ID3D11VertexShader> m_pVertexShader3D;
	ComPtr<ID3D11PixelShader> m_pPixelShader3D;
	ComPtr<ID3D11VertexShader> m_pVertexShader2D;
	ComPtr<ID3D11PixelShader> m_pPixelShader2D;

	ComPtr<ID3D11InputLayout> m_pVertexLayout2D;
	ComPtr<ID3D11InputLayout> m_pVertexLayout3D;

	ComPtr<ID3D11ShaderResourceView> m_pTexture;
private:

};

namespace {
	static BasicEffect* g_pInstance = nullptr;
}

BasicEffect::BasicEffect() {
	if (g_pInstance)
		throw std::exception("BasicEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect() {

}

BasicEffect::BasicEffect(BasicEffect&& moveFrom) noexcept {
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect& BasicEffect::operator=(BasicEffect&& moveFrom) noexcept {
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect& BasicEffect::Get() {
	if (!g_pInstance)
		throw std::exception("BasicEffect needs an instance!");
}

bool BasicEffect::InitAll(ID3D11Device* device) {
	if (!device)
		return false;

	if (!pImpl->m_pCBuffers.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	ComPtr<ID3DBlob> blob;

	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_2D.cso", L"HLSL\\Basic_VS_2D.hlsl", "VS_2D", "vs_5_0", blob.GetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pVertexShader2D.GetAddressOf()));
	HR(device->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout), blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexLayout2D.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_2D.cso", L"HLSL\\Basic_PS_2D.hlsl", "PS_2D", "ps_5_0", blob.GetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pPixelShader2D.GetAddressOf()));
	

	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_3D.cso", L"HLSL\\Basic_VS_3D.hlsl", "VS_3D", "vs_5_0", blob.GetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pVertexShader3D.GetAddressOf()));
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout), blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexLayout3D.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_3D.cso", L"HLSL\\Basic_PS_3D.hlsl", "PS_3D", "ps_5_0", blob.GetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pPixelShader3D.GetAddressOf()));

	pImpl->m_pCBuffers.assign({
		&pImpl->m_CBDrawing,
		&pImpl->m_CBFrame,
		&pImpl->m_CBStates,
		&pImpl->m_CBOnResize,
		&pImpl->m_CBRarely
		});

	for (auto& pBuffer : pImpl->m_pCBuffers) {
		HR(pBuffer->CreateBuffer(device));
	}

	D3D11SetDebugObjectName(pImpl->m_pVertexLayout2D.Get(), "VertexPosTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pVertexLayout3D.Get(), "VertexPosNormalTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[0]->cBuffer.Get(), "CBDrawing");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[1]->cBuffer.Get(), "CBStates");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[2]->cBuffer.Get(), "CBFrame");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[3]->cBuffer.Get(), "CBOnResize");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[4]->cBuffer.Get(), "CBRarely");
	D3D11SetDebugObjectName(pImpl->m_pVertexShader2D.Get(), "Basic_VS_2D");
	D3D11SetDebugObjectName(pImpl->m_pVertexShader3D.Get(), "Basic_VS_3D");
	D3D11SetDebugObjectName(pImpl->m_pPixelShader2D.Get(), "Basic_PS_2D");
	D3D11SetDebugObjectName(pImpl->m_pPixelShader3D.Get(), "Basic_PS_3D");

	return true;
}

void BasicEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext) {
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->m_pVertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->m_pPixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicEffect::SetRenderAlphaBlend(ID3D11DeviceContext* deviceContext) {
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->m_pVertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->m_pPixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFFFF);
}

void BasicEffect::SetRenderNoDoubleBlend(ID3D11DeviceContext* deviceContext, UINT stencilRef) {
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->m_pVertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->m_pPixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDoubleBlend.Get(), stencilRef);
	deviceContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
}

void BasicEffect::SetWriteStencilOnly(ID3D11DeviceContext* deviceContext, UINT stencilRef) {
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->m_pVertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->m_pPixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSWriteStencil.Get(), stencilRef);
	deviceContext->OMSetBlendState(RenderStates::BSNoColorWrite.Get(), nullptr, 0xFFFFFFFF);
}

void BasicEffect::SetRenderDefaultWithStencil(ID3D11DeviceContext* deviceContext, UINT stencilRef) {
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->m_pVertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSCullClockWise.Get());
	deviceContext->PSSetShader(pImpl->m_pPixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSDrawWithStencil.Get(), stencilRef);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicEffect::SetRenderAlphaBlendWithStencil(ID3D11DeviceContext* deviceContext, UINT stencilRef) {
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->m_pVertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->m_pPixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSDrawWithStencil.Get(), stencilRef);
	deviceContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
}

void BasicEffect::Set2DRenderDefault(ID3D11DeviceContext* deviceContext) {
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexLayout2D.Get());
	deviceContext->VSSetShader(pImpl->m_pVertexShader2D.Get(), nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->m_pPixelShader2D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicEffect::Set2DRenderAlphaBlend(ID3D11DeviceContext* deviceContext) {
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexLayout2D.Get());
	deviceContext->VSSetShader(pImpl->m_pVertexShader2D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->m_pPixelShader2D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->m_CBDrawing;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->m_CBOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetReflectionMatrix(FXMMATRIX R)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.reflection = XMMatrixTranspose(R);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetShadowMatrix(FXMMATRIX S)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.shadow = XMMatrixTranspose(S);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetRefShadowMatrix(DirectX::FXMMATRIX RefS)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.refShadow = XMMatrixTranspose(RefS);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight& dirLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight& pointLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight& spotLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material& material)
{
	auto& cBuffer = pImpl->m_CBDrawing;
	cBuffer.data.material = material;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTexture(ID3D11ShaderResourceView* texture)
{
	pImpl->m_pTexture = texture;
}

void BasicEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetReflectionState(bool isOn)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.isReflection = isOn;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetShadowState(bool isOn)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.isShadow = isOn;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ID3D11DeviceContext* deviceContext)
{
	auto& pCBuffers = pImpl->m_pCBuffers;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	pCBuffers[2]->BindVS(deviceContext);
	pCBuffers[3]->BindVS(deviceContext);
	pCBuffers[4]->BindVS(deviceContext);

	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);
	pCBuffers[4]->BindPS(deviceContext);

	// 设置纹理
	deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTexture.GetAddressOf());

	if (pImpl->m_IsDirty)
	{
		pImpl->m_IsDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}



