#pragma once
#include "stdafx.h"
#include "window.h"
#include "Simulation.hpp"
#include "camera.h"

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

struct Vertex
{
    float position[3];
    float color[4];
};

class RenderEngine {
public:
    virtual void init( Window& ) = 0;
    virtual void shutdown() = 0;
    virtual void render() = 0;

    float m_ElapsedTime = 0.0;

    // Simulation
    Simulation                                        m_Simulation;
};

class D3D12RenderEngine : public RenderEngine {
public:
    void init( Window& ) override;
    void shutdown() override;
    void render() override;

private:
    // TODO: Typedef the Microsoft Verbosity out.
    // Pipeline State
    CD3DX12_VIEWPORT                                  m_ViewPort;
    CD3DX12_RECT                                      m_ScissorRect;
    Microsoft::WRL::ComPtr<ID3D12Device>              m_Device;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_CommandListAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>        m_CommandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3>           m_SwapChain;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
    Microsoft::WRL::ComPtr<ID3D12Resource>            m_RenderTargets[3];
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       m_RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       m_PipelineState;

    //Synchronization
    Microsoft::WRL::ComPtr<ID3D12Fence>               m_Fence;
    UINT64                                            m_FenceValue;
    HANDLE                                            m_FenceEvent;
    UINT                                              m_FrameIndex;

    // Heaps
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      m_RTVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      m_SRVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      m_DSVHeap;
    UINT                                              m_RTVDescriptorSize;

    // Scene
    std::vector<Vertex>                               m_VertexBufferData;
    std::vector<uint32_t>                             m_IndexBufferData;
    std::vector<uint32_t>                             m_ConstantBufferData;
    Microsoft::WRL::ComPtr<ID3D12Resource>            m_VertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>            m_IndexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>            m_ConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>            m_DepthStencilBuffer;
    D3D12_VERTEX_BUFFER_VIEW                          m_VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW                           m_IndexBufferView;
    D3D12_CPU_DESCRIPTOR_HANDLE                       m_ConstantBufferView;
    D3D12_CPU_DESCRIPTOR_HANDLE                       m_DSBufferView;


    // Camera
    Camera                                            m_CameraWorld;
    Camera                                            m_CameraShadows;

    void    InitializeUserInterface(const Window&);
    void    UpdateTransforms();
    void    RecordCommands();
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