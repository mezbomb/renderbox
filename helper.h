#pragma once
#include "stdafx.h"

struct Vertex
{
    float position[3];
    float color[3];
};

struct ModelViewProjection
{
    DirectX::XMMATRIX projectionMatrix;
    DirectX::XMMATRIX modelMatrix;
    DirectX::XMMATRIX viewMatrix;
};

class CPUDescriptorHandle
{
public:
    CPUDescriptorHandle() = default;
    CPUDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& other) : m_Handle(other) {}

    operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_Handle; }
    operator const D3D12_CPU_DESCRIPTOR_HANDLE* () const { return &m_Handle; }
    operator bool() const { return m_Handle.ptr != 0; }

    CPUDescriptorHandle& operator++()    // Prefix increment
    {
        m_Handle.ptr += m_IncrementSize;
        return *this;
    }

    CPUDescriptorHandle operator++(int)  // Postfix increment
    {
        CPUDescriptorHandle tmp = *this;
        ++(*this);
        return tmp;
    }

    CPUDescriptorHandle offset(const int& n, const size_t& size) {
        m_Handle.ptr += (n * size);
        return *this;
    }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE m_Handle = {};
    static size_t const m_IncrementSize = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
        /* Sampler descriptor size */ 0 :
        /* CBV/SRV/UAV descriptor size */ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == D3D12_DESCRIPTOR_HEAP_TYPE_RTV ?
        /* RTV descriptor size */ 0 :
        /* DSV descriptor size */ D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
};

class Simulation
{
public:
    std::chrono::steady_clock::time_point m_StartTime;
    std::chrono::steady_clock::time_point m_EndTime;
    float m_ElapsedTime;
};

