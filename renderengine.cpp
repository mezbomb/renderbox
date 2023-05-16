#pragma once
#include "renderengine.h"
#include "helper.h"

using namespace DirectX;

void D3D12RenderEngine::init(const Window& window) {
    HWND hwnd = window.getHWND();

#ifdef NOT_DEBUG
    {
        ID3D12Debug* debugController;
        DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
        debugController->Release();
    }
#endif

    //TODO: Adapter Selection
    DX_CHECK(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)));

    D3D12_FEATURE_DATA_D3D12_OPTIONS options;
    ZeroMemory(&options, sizeof(options));
    DX_CHECK(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, reinterpret_cast<void*>(&options), sizeof(options)));

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    DX_CHECK(m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue)));

    DX_CHECK(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandListAllocator)));

    DX_CHECK(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
    m_FenceValue = 1;

    m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (nullptr == m_FenceEvent) { throw( std::exception()); } // something went wrong we can't synchronize.

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 3; // TODO: Allow other buffering types.
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // might want sequential when moving to use last frame as a motion vector?
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags = 0; // TODO: Check out options later.

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory = nullptr;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain = nullptr;
    DX_CHECK(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
    DX_CHECK(factory->CreateSwapChainForHwnd(
        m_CommandQueue.Get(),
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain));
    factory->Release();

    if (swapChain != nullptr) {
        DX_CHECK(swapChain.As(&m_SwapChain));
        m_SwapChain->ResizeBuffers(3, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NumDescriptors = 3;
    DX_CHECK(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)));

    m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < 3; i++) {
        DX_CHECK(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i])));
        m_Device->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_RTVDescriptorSize);
    }

    m_ViewPort.TopLeftX = 0.0f;
    m_ViewPort.TopLeftY = 0.0f;
    m_ViewPort.Width = 1280;
    m_ViewPort.Height = 720;
    m_ViewPort.MaxDepth = 1.0f;
    m_ViewPort.MinDepth = 0.0f;

    m_ScissorRect.left = 0;
    m_ScissorRect.top = 0;
    m_ScissorRect.right = 1280;
    m_ScissorRect.bottom = 720;

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvHeapDesc.NumDescriptors = 1;
    DX_CHECK(m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NumDescriptors = 1;
    DX_CHECK(m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

    CD3DX12_DEPTH_STENCIL_DESC depthStencilStateDesc = CD3DX12_DEPTH_STENCIL_DESC();
    CD3DX12_RESOURCE_DESC depthStencilBufferDesc = {};
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilBufferDesc.Width = 1280;
    depthStencilBufferDesc.Height = 720;
    depthStencilBufferDesc.DepthOrArraySize = 1;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    depthStencilBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    D3D12_DEPTH_STENCIL_VALUE dsValue = { 1.0f, 0 };
    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthClearValue.DepthStencil = dsValue;

    CD3DX12_HEAP_PROPERTIES dsvHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DX_CHECK(m_Device->CreateCommittedResource(
        &dsvHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilBufferDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&m_DepthStencilBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvViewDesc = {};
    dsvViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvViewDesc.Flags = D3D12_DSV_FLAG_NONE;

    // Don't like that it doesn't return an HRESULT...
    m_DSBufferView = m_DSVHeap.Get()->GetCPUDescriptorHandleForHeapStart();
    m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &dsvViewDesc, m_DSBufferView);

    // Graphics initialization is complete.  We could move the rest to asset loading / pipeline state.
    CD3DX12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].InitAsConstantBufferView(0);
    //rootParameters[1].InitAsDescriptorTable(1, &srvHeapDesc);
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    DX_CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DX_CHECK(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    // Compile and load shaders.
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
    const wchar_t* path = L"C:\\Users\\jerem\\source\\repos\\renderbox\\shaders.hlsl";
    DX_CHECK(D3DCompileFromFile(path, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexShader, nullptr));
    DX_CHECK(D3DCompileFromFile(path, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShader, nullptr));

    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // TODO: Pulled raster state out to turn off culling for debugging depth buffer.
    CD3DX12_RASTERIZER_DESC rasterStateDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterStateDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterStateDesc.FrontCounterClockwise = TRUE;
    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDesc, _countof(inputElementDesc) };
    psoDesc.pRootSignature = m_RootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = rasterStateDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    DX_CHECK(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)));

    // Create Command List
    DX_CHECK(m_Device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_CommandListAllocator.Get(),
        m_PipelineState.Get(),
        IID_PPV_ARGS(&m_CommandList)));

    DX_CHECK(m_CommandList->Close());

    Vertex vertices[] =
    {
        // Front face
        { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } },   // Bottom-left-front
        { { -0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } },    // Top-left-front
        { { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } },     // Top-right-front
        { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } },    // Bottom-right-front

        // Back face
        { { -0.5f, -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },    // Bottom-left-back
        { { -0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },     // Top-left-back
        { { 0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },       // Top-right-back
        { { 0.5f, -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },     // Bottom-right-back

        // Left face
        { { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } },    // Bottom-left-back
        { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } },     // Top-left-back
        { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } },    // Top-left-front
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } },   // Bottom-left-front

        // Right face
        { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } },    // Bottom-right-front
        { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } },      // Top-right-front
        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } },       // Top-right-back
        { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } },     // Bottom-right-back

        // Top face
        { { -0.5f, 0.5f, -0.5f }, {0.0f, 0.0f, 0.0f, 1.0f } },     // Top-left-front
        { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f, 1.0f } },      // Top-left-back
        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f, 1.0f } },        // Top-right-back
        { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f, 1.0f } },      // Top-right-front

        // Bottom face
        { { -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f, 1.0f } },    // Bottom-left-front
        { { -0.5f, -0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f, 1.0f } },     // Bottom-left-back
        { { 0.5f, -0.5f, 0.5f }, {  0.5f, 0.5f, 0.5f, 1.0f } },       // Bottom-right-back
        { { 0.5f, -0.5f, -0.5f }, {  0.5f, 0.5f, 0.5f, 1.0f } },     // Bottom-right-front
    };

    const UINT vertexBufferSize = sizeof(vertices);

    // TODO: from MSFT tutorial.
    // Note: using upload heaps to transfer static data like vert buffers is not 
    // recommended. Every time the GPU needs it, the upload heap will be marshalled 
    // over. Please read up on Default Heap usage. An upload heap is used here for 
    // code simplicity and because there are very few verts to actually transfer.
    CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   resourceDesc   = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    DX_CHECK(m_Device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_VertexBuffer)));

    // Copy the triangle data to the vertex buffer.
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    DX_CHECK(m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, vertices, sizeof(vertices));
    m_VertexBuffer->Unmap(0, nullptr);

    // Initialize the vertex buffer view.
    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.StrideInBytes = sizeof(Vertex);
    m_VertexBufferView.SizeInBytes = vertexBufferSize;

    // do the same for index buffer
    // Indices for a cube with two triangles per face
    UINT indices[] =
    {
        // Front face
        2, 1, 0,    // Triangle 1
        0, 3, 2,    // Triangle 2

        // Back face
        4, 5, 6,    // Triangle 1
        6, 7, 4,    // Triangle 2

        // Left face
        10, 9, 8,   // Triangle 1
        8, 11, 10,  // Triangle 2

        // Right face
        14, 13, 12, // Triangle 1
        12, 15, 14, // Triangle 2

        // Top face
        18, 17, 16,    // Triangle 1
        16, 19, 18,    // Triangle 2

        // Bottom face
        20, 21, 22,    // Triangle 1
        22, 23, 20,     // Triangle 2
    };

    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices));
    m_Device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_IndexBuffer));

    void* pIndexData = nullptr;
    m_IndexBuffer->Map(0, nullptr, &pIndexData);
    memcpy(pIndexData, indices, sizeof(indices));
    m_IndexBuffer->Unmap(0, nullptr);

    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_IndexBufferView.SizeInBytes = sizeof(indices);

    CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX));
    // Create the constant buffer resource
    m_Device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_ConstantBuffer));

    InitializeUserInterface(window);

    // Begin Simulation
    m_Simulation.m_StartTime = std::chrono::high_resolution_clock::now();

}

void D3D12RenderEngine::InitializeUserInterface(const Window& window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.DeltaTime = 0.0f;
    io.DisplaySize = ImVec2(static_cast<float>(window.getWidth()), static_cast<float>(window.getHeight()));
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark(); // Everybody loves the dark :)

    ImGui_ImplWin32_Init(window.getHWND());
    ImGui_ImplDX12_Init(
        m_Device.Get(),
        3,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        m_SRVHeap.Get(),
        m_SRVHeap->GetCPUDescriptorHandleForHeapStart(),
        m_SRVHeap->GetGPUDescriptorHandleForHeapStart());
}

void D3D12RenderEngine::shutdown() {

}

void D3D12RenderEngine::RecordCommands(){
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    DX_CHECK(m_CommandListAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    DX_CHECK(m_CommandList->Reset(m_CommandListAllocator.Get(), m_PipelineState.Get()));

    // Set necessary state.
    m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
    UpdateTransforms();

    m_CommandList->RSSetViewports(1, &m_ViewPort);
    m_CommandList->RSSetScissorRects(1, &m_ScissorRect);
    m_CommandList->ClearDepthStencilView(m_DSBufferView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    auto renderTarget = m_RenderTargets[m_FrameIndex].Get();
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Indicate that the back buffer will be used as a render target.
    m_CommandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_RTVHeap->GetCPUDescriptorHandleForHeapStart(),
        m_FrameIndex,
        m_RTVDescriptorSize);

    m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &m_DSBufferView);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    m_CommandList->IASetIndexBuffer(&m_IndexBufferView);

    // Issue cube face draw commands.
    constexpr UINT numFaces = 6;
    constexpr UINT indicesPerFace = 6;
    for (UINT faceIndex = 0; faceIndex < numFaces; ++faceIndex) {
        UINT startIndex = faceIndex * indicesPerFace;
        m_CommandList->DrawIndexedInstanced(indicesPerFace, 1, startIndex, 0, 0);
    }

    // Render Dear ImGui graphics
    ID3D12DescriptorHeap* pHeap = m_SRVHeap.Get();
    m_CommandList->SetDescriptorHeaps(1, &pHeap );
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());


    // Indicate that the back buffer will now be used to present.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);

    m_CommandList->ResourceBarrier(1, &barrier);

    DX_CHECK(m_CommandList->Close());
}

void D3D12RenderEngine::UpdateTransforms() {

    float speed = 0.001f;
    float rotationAngleX = speed * m_ElapsedTime;
    float rotationAngleY = speed * m_ElapsedTime;
    float rotationAngleZ = speed * m_ElapsedTime;
    float bottomSideRotationAngleY = XM_PIDIV4;  // 45deg
    rotationAngleY += bottomSideRotationAngleY;
    XMMATRIX rot = XMMatrixRotationX(rotationAngleX) * XMMatrixRotationY(rotationAngleY) * XMMatrixRotationZ(rotationAngleZ);

    XMMATRIX modelMatrix = XMMatrixIdentity(); // Identity matrix for the model transformation
    modelMatrix = modelMatrix * rot; //apply rotation to the model

    XMVECTOR eyePosition = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);  // Camera position
    XMVECTOR focusPosition = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);  // Camera focus point
    XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);  // Up direction

    XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);  // View matrix

    float fovAngleY = XMConvertToRadians(45.0f);  // Field of view angle in radians
    float nearPlane = 0.1f;  // Near clipping plane distance
    float farPlane = 100.0f;  // Far clipping plane distance

    float aspectRatio = 1280.0f / 720.0f;
    XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearPlane, farPlane);  // Projection matrix
    XMMATRIX mvpMatrix = modelMatrix * viewMatrix * projectionMatrix;

    // Map the constant buffer for CPU write access
    XMMATRIX* cbuffer;
    m_ConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&cbuffer));
    *cbuffer = mvpMatrix;

    // Unmap the constant buffer
    m_ConstantBuffer->Unmap(0, nullptr);

    // Bind the constant buffer to the appropriate shader register
    m_CommandList->SetGraphicsRootConstantBufferView(0, m_ConstantBuffer->GetGPUVirtualAddress());
}

void D3D12RenderEngine::render() {
    m_Simulation.m_EndTime = std::chrono::high_resolution_clock::now();

    float time =
        std::chrono::duration<float, std::milli>(m_Simulation.m_EndTime - m_Simulation.m_StartTime).count();
    if (time < (1000.0f / 60.0f))
    {
        return;
    }

    m_Simulation.m_StartTime = std::chrono::high_resolution_clock::now();
    m_ElapsedTime += time;

    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime += time;
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }
    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
    ImGui::Render();

    // TODO: Refactor to allow recording from other threads.
    RecordCommands();

    ID3D12CommandList* ppCommandLists[] = { m_CommandList.Get()};
    m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)m_CommandList.Get());
    }

    // 1 means we wait for vsync, the 0 is for flags.
    m_SwapChain->Present(1, 0);

    const UINT64 fence = m_FenceValue;
    DX_CHECK(m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue));
    m_FenceValue++;

    if (m_Fence->GetCompletedValue() < fence)
    {
        DX_CHECK(m_Fence->SetEventOnCompletion(fence, m_FenceEvent));
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}