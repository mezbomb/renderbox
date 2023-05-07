#pragma once
#include "stdafx.h"
#include "window.h"

#define DX_CHECK(expr) DX::ThrowIfFailed(expr, __FILE__, __LINE__)

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr, const char* file, int line)
	{
		if (FAILED(hr))
		{
			throw std::runtime_error(std::string(file) + "(" + std::to_string(line) + "): HRESULT = 0x" + std::to_string(hr));
		}
	}
} // namespace DX

enum ENGINE_TYPE {
	ENGINE_D3D12,
	ENGINE_VULKAN
};

class RenderEngine {
public:
	virtual void init( const Window& ) = 0;
	virtual void shutdown() = 0;
	virtual void render() = 0;
};

class D3D12RenderEngine : public RenderEngine {
public:
	void init( const Window& ) override;
	void shutdown() override;
	void render() override;

private:
	// TODO: Typedef the Microsoft Verbosity out.
	Microsoft::WRL::ComPtr<ID3D12Device>              m_Device;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_CommandListAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>        m_CommandQueue;
	Microsoft::WRL::ComPtr<IDXGIDevice2>              m_DXGIDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain>            m_SwapChain;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
	Microsoft::WRL::ComPtr<ID3D12Resource>            m_RenderTargets[3];

	//Synchronization
	Microsoft::WRL::ComPtr<ID3D12Fence>               m_Fence;
	UINT64                                            m_FenceValue;
	HANDLE                                            m_FenceEvent;

	// Heaps
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      m_RTVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      m_ConstantBufferHeap;

	// Scene
	std::vector<Vertex>                               m_VertexBufferData;
	std::vector<uint32_t>                             m_IndexBufferData;
	std::vector<uint32_t>                             m_ConstantBufferData;
	Microsoft::WRL::ComPtr<ID3D12Resource>            m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>            m_IndexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>            m_ConstantBuffer;
	D3D12_VERTEX_BUFFER_VIEW                          m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW							  m_IndexBufferView;
	D3D12_CPU_DESCRIPTOR_HANDLE                       m_ConstantBufferView;
	ModelViewProjection                               m_ModelViewProjectionMatrices;

	// Simulation
	Simulation                                        m_Simulation;


	void    WaitForCommandQueueFence() {};
	HRESULT ResizeSwapChain() {};
};

class RenderEngineFactory {
public:
	static RenderEngine* CreateRenderEngine(const ENGINE_TYPE& engineType) {
		if (engineType == ENGINE_D3D12) {
			return new D3D12RenderEngine();
		}

		// TODO: Add other engines.
		return nullptr;
	}
};