#include "renderengine.h"

void D3D12RenderEngine::init( const Window& window ) {
	HWND hwnd = window.getHWND();

	DX_CHECK( D3D12CreateDevice( nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&m_Device) );

	D3D12_FEATURE_DATA_D3D12_OPTIONS options;
	DX_CHECK(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, reinterpret_cast<void*>(&options), sizeof(options)));
	DX_CHECK(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_CommandListAllocator	));

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	DX_CHECK(m_Device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_CommandQueue));

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 3; // TODO: Allow other buffering types.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = FALSE; // TODO: Allow for other types.
	swapChainDesc.Flags = 0; // TODO: Check out options later.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // might want sequential when moving to use last frame as a motion vector?

	IDXGIFactory2* pDXGIFactory = nullptr;
	DX_CHECK(CreateDXGIFactory(__uuidof(IDXGIFactory2), (void**)&pDXGIFactory));
	DX_CHECK(pDXGIFactory->CreateSwapChain(m_CommandQueue.Get(), &swapChainDesc, (IDXGISwapChain**)m_SwapChain.GetAddressOf()));
	pDXGIFactory->Release();

}

void D3D12RenderEngine::shutdown() {

}

void D3D12RenderEngine::render() { }
