#pragma once

#include <wrl/client.h>
#include <d3d11_1.h>

class RenderStates {
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	static bool IsInit();

	static void InitAll(ID3D11Device* device);

public:
	static ComPtr<ID3D11RasterizerState> RSWireframe;
	static ComPtr<ID3D11RasterizerState> RSNoCull;

	static ComPtr<ID3D11SamplerState> SSLinearWrap;
	static ComPtr<ID3D11SamplerState> SSAnistropicWrap;

	static ComPtr<ID3D11BlendState> BSNoColorWrite;
	static ComPtr<ID3D11BlendState> BSTransparent;
	static ComPtr<ID3D11BlendState> BSAlphaToCoverage;
};