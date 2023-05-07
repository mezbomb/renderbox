#include "renderengine.h"

using namespace DirectX;

void D3D12RenderEngine::init(const Window& window) {
	m_FenceValue = 1337;
	HWND hwnd = window.getHWND();

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
	DX_CHECK(CreateDXGIFactory2(0, IID_PPV_ARGS(&pDXGIFactory)));
	DX_CHECK(pDXGIFactory->CreateSwapChain(m_CommandQueue.Get(), &swapChainDesc, (IDXGISwapChain**)m_SwapChain.GetAddressOf()));
	pDXGIFactory->Release();

	if (m_SwapChain != nullptr) {
		m_SwapChain->ResizeBuffers(3, 640, 480, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NumDescriptors = 3;
	DX_CHECK(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)));

	auto rtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CPUDescriptorHandle rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < 3; i++) {
		DX_CHECK(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i])));
		m_Device->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.offset(1, rtvDescriptorSize);
	}

	ID3D12RootSignature* rootSignature;
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))){
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_DESCRIPTOR_RANGE ranges[1];
	ranges[0].BaseShaderRegister = 0;
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[0].NumDescriptors = 1;
	ranges[0].RegisterSpace = 0;
	ranges[0].OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER rootParameters[1];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;

	ID3DBlob* signature;
	ID3DBlob* error;

	try {
		DX_CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	}
	catch (std::exception e) {
		DX_CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error));
	}

	if (signature) {
		signature->Release();
		signature = nullptr;
	}

	// Upload Heap
	ID3D12Resource* uploadBuffer;
	std::vector<unsigned char> sourceData;

	D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 1u;
	uploadHeapProperties.VisibleNodeMask = 1u;

	D3D12_RESOURCE_DESC uploadBufferDesc = {};
	uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	uploadBufferDesc.MipLevels = 1;
	uploadBufferDesc.DepthOrArraySize = 1;
	uploadBufferDesc.Height = 1;
	uploadBufferDesc.Width = 65536ull;
	uploadBufferDesc.Alignment = 65536ull;
	uploadBufferDesc.SampleDesc = { 1,0 };

	DX_CHECK( m_Device->CreateCommittedResource( 
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)));

	uint8_t* data = nullptr;
	D3D12_RANGE range{ 0, SIZE_T(65536ull) }; // we should avoid the hardcode and just define a chunk_size
	DX_CHECK(uploadBuffer->Map( 0, &range, reinterpret_cast<void**>(&data)));

	memcpy(data, sourceData.data(), sourceData.size());

	// Readback Heap
	ID3D12Resource* readbackBuffer;

	D3D12_HEAP_PROPERTIES heapPropsRead = { D3D12_HEAP_TYPE_READBACK,
										   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
										   D3D12_MEMORY_POOL_UNKNOWN, 1u, 1u };

	D3D12_RESOURCE_DESC resourceDescDimBuffer = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		2725888ull,
		1u,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1u, 0u},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE };

	DX_CHECK( m_Device->CreateCommittedResource(
		&heapPropsRead,
		D3D12_HEAP_FLAG_NONE,
		&resourceDescDimBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&readbackBuffer)));


	Vertex vertexBufferData[3] = { {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
								  {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
								  {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}} };


	const UINT vertexBufferSize = sizeof(vertexBufferData);

	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC vertexBufferResourceDesc;
	vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferResourceDesc.Alignment = 0;
	vertexBufferResourceDesc.Width = vertexBufferSize;
	vertexBufferResourceDesc.Height = 1;
	vertexBufferResourceDesc.DepthOrArraySize = 1;
	vertexBufferResourceDesc.MipLevels = 1;
	vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexBufferResourceDesc.SampleDesc.Count = 1;
	vertexBufferResourceDesc.SampleDesc.Quality = 0;
	vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	DX_CHECK(m_Device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexBuffer)));

	UINT8* pVertexDataBegin;

	D3D12_RANGE readRange;
	readRange.Begin = 0;
	readRange.End = 0;

	DX_CHECK(m_VertexBuffer->Map(0, &readRange,
		reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, vertexBufferData, sizeof(vertexBufferData));
	m_VertexBuffer->Unmap(0, nullptr);

	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(Vertex);
	m_VertexBufferView.SizeInBytes = vertexBufferSize;

	uint32_t indexBufferData[3] = { 0, 1, 2 };

	const UINT indexBufferSize = sizeof(indexBufferData);

	D3D12_RESOURCE_DESC indexBufferResourceDesc;
	indexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	indexBufferResourceDesc.Alignment = 0;
	indexBufferResourceDesc.Width = indexBufferSize;
	indexBufferResourceDesc.Height = 1;
	indexBufferResourceDesc.DepthOrArraySize = 1;
	indexBufferResourceDesc.MipLevels = 1;
	indexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	indexBufferResourceDesc.SampleDesc.Count = 1;
	indexBufferResourceDesc.SampleDesc.Quality = 0;
	indexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	indexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	DX_CHECK(m_Device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexBuffer)));

	UINT8* pIndexDataBegin;

	DX_CHECK(m_IndexBuffer->Map(0, &readRange,
		reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indexBufferData, sizeof(indexBufferData));
	m_IndexBuffer->Unmap(0, nullptr);

	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_IndexBufferView.SizeInBytes = indexBufferSize;

	UINT8* mappedConstantBuffer;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	DX_CHECK(m_Device->CreateDescriptorHeap(&heapDesc,
		IID_PPV_ARGS(&m_ConstantBufferHeap)));

	D3D12_RESOURCE_DESC cbResourceDesc;
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Alignment = 0;
	cbResourceDesc.Width = (sizeof(m_ModelViewProjectionMatrices) + 255) & ~255;
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.SampleDesc.Quality = 0;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	cbResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	DX_CHECK(m_Device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &cbResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ConstantBuffer)));
	m_ConstantBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_ConstantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes =
		(sizeof(m_ModelViewProjectionMatrices) + 255) & ~255; // CB size is required to be 256-byte aligned.

	D3D12_CPU_DESCRIPTOR_HANDLE
		cbvHandle(m_ConstantBufferHeap->GetCPUDescriptorHandleForHeapStart());
	cbvHandle.ptr = cbvHandle.ptr + m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

	m_Device->CreateConstantBufferView(&cbvDesc, cbvHandle);

	DX_CHECK(m_ConstantBuffer->Map(
		0, &readRange, reinterpret_cast<void**>(&mappedConstantBuffer)));
	memcpy(mappedConstantBuffer, &m_ModelViewProjectionMatrices, sizeof(m_ModelViewProjectionMatrices));
	m_ConstantBuffer->Unmap(0, &readRange);

	// Begin Simulation
	m_Simulation.m_StartTime = std::chrono::high_resolution_clock::now();

}

void D3D12RenderEngine::shutdown() {

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


	m_Simulation.m_ElapsedTime += 0.001f * time;
	m_Simulation.m_ElapsedTime = fmodf(m_Simulation.m_ElapsedTime, 6.283185307179586f);
	m_ModelViewProjectionMatrices.modelMatrix = XMMatrixRotationY(m_Simulation.m_ElapsedTime);

	D3D12_RANGE readRange;
	readRange.Begin = 0;
	readRange.End = 0;

	UINT8* mappedConstantBuffer;
	DX_CHECK(m_ConstantBuffer->Map(
		0, &readRange, reinterpret_cast<void**>(&mappedConstantBuffer)));
	memcpy(mappedConstantBuffer, &m_ModelViewProjectionMatrices, sizeof(m_ModelViewProjectionMatrices));
	m_ConstantBuffer->Unmap(0, &readRange);

	//setupCommands();

	ID3D12CommandList* ppCommandLists[] = { m_CommandList.Get()};
	m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// 1 means we wait for vsync, the 0 is for flags.
	//m_SwapChain->Present(1, 0);

	const UINT64 fence = m_FenceValue;
	DX_CHECK(m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue));
	m_FenceValue++;

	if (m_Fence->GetCompletedValue() < fence)
	{
		DX_CHECK(m_Fence->SetEventOnCompletion(fence, m_FenceEvent));
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}

	UINT frameIndex;
	m_SwapChain->GetLastPresentCount(&frameIndex);
	frameIndex %= 3; // triple buffered.
}


/* TODO Setup Command List and Resource Barrier for the swap chain?
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = nullptr; // WHere?
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_CommandList->ResourceBarrier(1, &barrier);
*/