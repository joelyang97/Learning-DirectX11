#pragma once

#include <wrl/client.h>
#include <string>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include "Mouse.h"
#include "KeyBoard.h"
#include "GameTimer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "winmm.lib")

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();

	HINSTANCE	AppInst() const;
	HWND		MainHnd() const;
	float		AspectRatio() const;

	int			Run();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt) = 0;
	virtual void DrawScene() = 0;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	bool InitMainWindow();
	bool InitDirect3D();

	void CalculateFrameStats();

protected:

	HINSTANCE	m_hAppInst;
	HWND		m_hMainWnd;
	bool		m_AppPaused;
	bool		m_Minimized;
	bool		m_Maximized;
	bool		m_Resizing;
	bool		m_Enable4xMsaa;
	UINT		m_4xMsaaQuality;

	GameTimer	m_Timer;

	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11Device>	m_pd3dDevice;
	ComPtr<ID3D11DeviceContext>	m_pd3dImmediateContext;
	ComPtr<IDXGISwapChain>	m_pSwapChain;

	ComPtr<ID3D11Device1>	m_pd3dDevice1;
	ComPtr<ID3D11DeviceContext1> m_pd3dImmediateContext1;
	ComPtr<IDXGISwapChain1>	m_pSwapChain1;

	ComPtr<ID3D11Texture2D>	m_pDepthStencilBuffer;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;

	D3D11_VIEWPORT	m_ScreenViewport;

	std::unique_ptr<DirectX::Mouse> m_pMouse;
	DirectX::Mouse::ButtonStateTracker m_MouseTracker;
	std::unique_ptr<DirectX::Keyboard> m_pKeyboard;
	DirectX::Keyboard::KeyboardStateTracker m_KeyboardTracker;

	std::wstring m_MainWndCaption;
	int m_ClientWidth;
	int m_ClientHeight;

private:

};

