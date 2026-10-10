#include "DX12Renderer.h"



constexpr Vertex vertices[] = {
	// Red cube: side = 0.6, center = (-0.15, 0.05, 0.0). Draw FIRST.
	// Vertices 0-3: near face; 4-7: far face. Darker colors help show the sides.
	{-0.45f,  0.35f, -0.30f, 1.0f, 0.2f, 0.1f, 1.0f},
	{ 0.15f,  0.35f, -0.30f, 1.0f, 0.2f, 0.1f, 1.0f},
	{ 0.15f, -0.25f, -0.30f, 1.0f, 0.2f, 0.1f, 1.0f},
	{-0.45f, -0.25f, -0.30f, 1.0f, 0.2f, 0.1f, 1.0f},
	{-0.45f,  0.35f,  0.30f, 0.5f, 0.05f, 0.03f, 1.0f},
	{ 0.15f,  0.35f,  0.30f, 0.5f, 0.05f, 0.03f, 1.0f},
	{ 0.15f, -0.25f,  0.30f, 0.5f, 0.05f, 0.03f, 1.0f},
	{-0.45f, -0.25f,  0.30f, 0.5f, 0.05f, 0.03f, 1.0f},

	// Blue cube: side = 0.6, center = (0.15, -0.05, 0.65). Draw LAST.
	// Vertices 8-11: near face; 12-15: far face.
	{-0.15f,  0.25f, 0.35f, 0.1f, 0.4f, 1.0f, 1.0f},
	{ 0.45f,  0.25f, 0.35f, 0.1f, 0.4f, 1.0f, 1.0f},
	{ 0.45f, -0.35f, 0.35f, 0.1f, 0.4f, 1.0f, 1.0f},
	{-0.15f, -0.35f, 0.35f, 0.1f, 0.4f, 1.0f, 1.0f},
	{-0.15f,  0.25f, 0.95f, 0.03f, 0.15f, 0.5f, 1.0f},
	{ 0.45f,  0.25f, 0.95f, 0.03f, 0.15f, 0.5f, 1.0f},
	{ 0.45f, -0.35f, 0.95f, 0.03f, 0.15f, 0.5f, 1.0f},
	{-0.15f, -0.35f, 0.95f, 0.03f, 0.15f, 0.5f, 1.0f}
};

constexpr uint16_t indices[] = {
	// Red cube: 6 faces, 2 triangles per face (36 indices).
	0, 1, 2,  0, 2, 3, // Near
	4, 6, 5,  4, 7, 6, // Far
	0, 3, 7,  0, 7, 4, // Left
	1, 5, 6,  1, 6, 2, // Right
	0, 4, 5,  0, 5, 1, // Top
	3, 2, 6,  3, 6, 7, // Bottom

	// Blue cube: same topology, vertex indices offset by 8.
	8, 9, 10,    8, 10, 11,  // Near
	12, 14, 13,  12, 15, 14, // Far
	8, 11, 15,   8, 15, 12,  // Left
	9, 13, 14,   9, 14, 10,  // Right
	8, 12, 13,   8, 13, 9,   // Top
	11, 10, 14,  11, 14, 15  // Bottom
};

using namespace Microsoft::WRL;

DX12Renderer::DX12Renderer() {

}

DX12Renderer::~DX12Renderer()
{
	if (fenceEvent != nullptr) {
		CloseHandle(fenceEvent);
	}
}

void DX12Renderer::initialize(HWND hwnd) {

	// initialize factory
	HRESULT hr = CreateDXGIFactory2(
		0,
		IID_PPV_ARGS(&factory)
	);
	ThrowIfFailed(hr, "CreateDXGIFactory2");

	// initialize and enum adapters

	for (UINT i = 0;; ++i) {

		hr = factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf());

		if (hr == DXGI_ERROR_NOT_FOUND) {
			break;
		}
		ThrowIfFailed(hr, "IDXGIFactory6::EnumAdapters1");

		DXGI_ADAPTER_DESC1 desc;
		ThrowIfFailed(adapter->GetDesc1(&desc), "IDXGIAdapter1::GetDesc1");

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {  // skip GPU from software simulation
			adapter.Reset();
			continue;
		}

		break;
	}

	if (!adapter)
	{
		throw std::runtime_error("No suitable hardware adapter found.");
	}

	// create device

	hr = D3D12CreateDevice(
		adapter.Get(),
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&device)
	);
	ThrowIfFailed(hr, "D3D12CreateDevice");


	// -----------  Create command Queue  ---------------------

	// ComPtr<ID3D12CommandQueue> commandQueue;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};

	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

	ThrowIfFailed(hr, "ID3D12Device::CreateCommandQueue");

	// ----------  Create swap chain  ------------------
	// ComPtr<IDXGISwapChain4> swapChain;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

	RECT clientRect{};
	if (!GetClientRect(hwnd, &clientRect)) {
		throw std::system_error(GetLastError(), std::system_category(), "GetClientRect failed");
	}

	mWidth = clientRect.right - clientRect.left;
	mHeight = clientRect.bottom - clientRect.top;

	swapChainDesc.Width = mWidth;
	swapChainDesc.Height = mHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> tempSwapChain;

	hr = factory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&tempSwapChain
	);

	ThrowIfFailed(hr, "IDXGIFactory6::CreateSwapChainForHwnd");

	hr = tempSwapChain.As(&swapChain);
	ThrowIfFailed(hr, "Query IDXGISwapChain4");

	swapChain->GetCurrentBackBufferIndex();


	hr = factory->MakeWindowAssociation(
		hwnd,
		DXGI_MWA_NO_ALT_ENTER
	); // ban Alt + Enter as Fullscreen
	ThrowIfFailed(hr, "IDXGIFactory6::MakeWindowAssociation");


	// --------  Get back buffer and create RTV  ---------
	// since we have a double buffering system, we should create two back buffers



	// Create a RTV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;


	hr = device->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(&rtvHeap)
	);
	ThrowIfFailed(hr, "ID3D12Device::CreateDescriptorHeap (RTV)");

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart(); // get start point of rtv

	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (UINT i = 0; i < FrameCount; ++i) {
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));

		ThrowIfFailed(hr, "IDXGISwapChain4::GetBuffer");

		device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);

		rtvHandle.ptr += rtvDescriptorSize;
	}


	// ----------- Create CommandAllocator and CommandList -------------
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

	ThrowIfFailed(hr, "ID3D12Device::CreateCommandAllocator");

	hr = device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&commandList)
	);
	ThrowIfFailed(hr, "ID3D12Device::CreateCommandList");

	hr = commandList->Close();
	ThrowIfFailed(hr, "ID3D12GraphicsCommandList::Close (initial)");

	// ----------- Creare Fence --------
	hr = device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence)
	);

	ThrowIfFailed(hr, "ID3D12Device::CreateFence");

	// ----------- Create Window Event ---------

	fenceEvent = CreateEvent(
		nullptr,
		FALSE,
		FALSE,
		nullptr
	);

	if (fenceEvent == nullptr) {
		throw std::system_error(GetLastError(), std::system_category(), "CreateEvent failed");
	}

	// ----------- Create RootSignature -------------
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_ROOT_PARAMETER transformParameter = {};
	transformParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	transformParameter.Descriptor.ShaderRegister = 0;
	transformParameter.Descriptor.RegisterSpace = 0;
	transformParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &transformParameter;

	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;

	// Serialize
	ComPtr<ID3DBlob> serializedRootSignature;
	hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSignature,
		&errorBlob
	);
	ThrowIfFailed(hr, "D3D12SerializeRootSignature");

	// create root signature
	hr = device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	ThrowIfFailed(hr, "D3D12RootSignature");

	// ----------- Complie shaders ------------
	hr = D3DCompileFromFile(
		L"shader.hlsl",
		nullptr,
		nullptr,
		"VSMain",
		"vs_5_0",
		0,
		0,
		&vertexShader,
		&errorBlob
	);

	ThrowIfFailed(hr, "Complie Vertex Shader");


	hr = D3DCompileFromFile(
		L"shader.hlsl",
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		0,
		0,
		&pixelShader,
		&errorBlob
	);

	ThrowIfFailed(hr, "Compile FragmentShader");

	// -------------- Input Layout and Vertex Buffer View ----------
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		{
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};



	// ------------ Depth Test -------------
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	heapDesc.NumDescriptors = 1;

	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	heapDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(&dsvHeap)
	),
		"Create DSV Heap"
	);

	// ---------- PSO ------------

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	psoDesc.PS = { pixelShader->GetBufferPointer() , pixelShader->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;

	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.IndependentBlendEnable = FALSE;


	// Depth Stencil Test
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE; // Set FALSE to see blue overwrite red in the overlap.
	depthStencilDesc.StencilEnable = FALSE;

	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState = depthStencilDesc;

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	psoDesc.SampleMask = UINT_MAX;

	psoDesc.NumRenderTargets = 1;

	psoDesc.RTVFormats[0] =
		DXGI_FORMAT_R8G8B8A8_UNORM;

	psoDesc.SampleDesc.Count = 1;

	psoDesc.InputLayout =
	{
		inputLayout,
		_countof(inputLayout)
	};

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
	ThrowIfFailed(hr, "Create PSO");



	// --------------- Vertex Buffer ----------------

	UINT vertexBufferSize = sizeof(vertices); // vertices is defined in dataStructure.h (Temporarily)

	// Upload Heap
	D3D12_HEAP_PROPERTIES heapProperties = {};

	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // important!
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;


	// Resource description

	D3D12_RESOURCE_DESC resourceDesc = {};

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = vertexBufferSize; // width is just buffer size, not any width
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;

	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;

	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;

	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// vertex buffer resource

	hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)
	);
	ThrowIfFailed(hr, "Buffer Resource");



	// put vertices data into buffer

	void* mappedData = nullptr;

	vertexBuffer->Map(
		0,
		nullptr,
		&mappedData
	);

	memcpy(
		mappedData,
		vertices,
		vertexBufferSize
	);

	vertexBuffer->Unmap(0, nullptr);


	// describe format of vertex
	vertexBufferView = {};

	vertexBufferView.BufferLocation =
		vertexBuffer->GetGPUVirtualAddress();

	vertexBufferView.SizeInBytes =
		vertexBufferSize;

	vertexBufferView.StrideInBytes =
		sizeof(Vertex);

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(mWidth);
	viewport.Height = static_cast<float>(mHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(mWidth);
	scissorRect.bottom = static_cast<LONG>(mHeight);

	// ----------- IndexBuffer ---------------

	UINT bufferSize = sizeof(indices);

	resourceDesc = {};

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = bufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer)
	);
	ThrowIfFailed(hr, "Create Index Buffer Resource");

	mappedData = nullptr;

	D3D12_RANGE readRange = { 0, 0 };

	hr = indexBuffer->Map(
		0,
		&readRange,
		&mappedData
	);

	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to map index buffer");
	}

	memcpy(mappedData, indices, sizeof(indices));

	D3D12_RANGE writtenRange = { 0, sizeof(indices) };
	indexBuffer->Unmap(0, &writtenRange);


	indexBufferView.BufferLocation =
		indexBuffer->GetGPUVirtualAddress();

	indexBufferView.SizeInBytes =
		sizeof(indices);

	indexBufferView.Format =
		DXGI_FORMAT_R16_UINT;




	// ------------- Constant Bufffer ----------------
	heapProperties = {};

	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	resourceDesc = {};

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	const UINT constantBufferSize =
		(sizeof(TransformConstants) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)
		& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
	resourceDesc.Width = constantBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mConstantBuffer)
	);

	ThrowIfFailed(hr, "Create Constant Buffer");



	readRange = { 0, 0 };

	mappedData = nullptr;

	hr = mConstantBuffer->Map(
		0,
		&readRange,
		&mappedData
	);

	ThrowIfFailed(hr, "Map to Constant Buffer");
	mConstantBufferMapped = static_cast<uint8_t*>(mappedData);





	heapProperties = {};

	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC backDesc =
		renderTargets[0]->GetDesc();

	D3D12_RESOURCE_DESC depthDesc = {};

	depthDesc.Dimension =
		D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	depthDesc.Alignment = 0;

	depthDesc.Width = backDesc.Width;
	depthDesc.Height = backDesc.Height;

	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;

	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;

	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;

	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	depthDesc.Flags =
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;


	// Optimizer Clear Value
	D3D12_CLEAR_VALUE clearValue = {};

	clearValue.Format = DXGI_FORMAT_D32_FLOAT;

	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;


	// Create Committed Resource
	ThrowIfFailed(
		device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&depthBuffer)
		),
		"Create Depth Buffer"
	);

	// Create the view only after both the DSV heap and depth buffer exist.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Texture2D.MipSlice = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		dsvHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHandle);




}



void DX12Renderer::render() {
	UINT frameIndex = swapChain->GetCurrentBackBufferIndex();

	ThrowIfFailed(commandAllocator->Reset(), "ID3D12CommandAllocator::Reset");
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr), "ID3D12GraphicsCommandList::Reset");

	D3D12_RESOURCE_BARRIER barrier = {};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	barrier.Transition.pResource = renderTargets[frameIndex].Get();

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &barrier);

	// RTV cooresponding to Current back buffer
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	rtvHandle.ptr += frameIndex * rtvDescriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

	// Output Merger
	commandList->OMSetRenderTargets(
		1,
		&rtvHandle,
		FALSE,
		&dsvHandle
	);

	// Clear Render Target
	commandList->ClearRenderTargetView(
		rtvHandle,
		clearColor,
		0,
		nullptr
	);


	// Clear Depth Buffer
	commandList->ClearDepthStencilView(
		dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f,
		0,
		0,
		nullptr
	);

	// ---------- Viewport ---------
	commandList->RSSetViewports(1, &viewport);

	// ---------- Scissor Rect ----------
	commandList->RSSetScissorRects(1, &scissorRect);

	// Input Assembler
	commandList->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);

	commandList->IASetVertexBuffers(
		0,
		1,
		&vertexBufferView
	);

	commandList->IASetIndexBuffer(&indexBufferView); // Index Buffer / EBO

	commandList->SetPipelineState(pipelineState.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	commandList->SetGraphicsRootConstantBufferView(0, mConstantBuffer->GetGPUVirtualAddress());

	auto now = std::chrono::steady_clock::now();

	float elapsedSeconds =
		std::chrono::duration<float>(
			now - mStartTime
		).count();

	UpdateConstantBuffer(elapsedSeconds);

	commandList->DrawIndexedInstanced(
		static_cast<UINT>(_countof(indices)),
		1,
		0,
		0,
		0
	);

	// Transition back only after all rendering commands.
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);




	// Stop Recording
	ThrowIfFailed(commandList->Close(), "ID3D12GraphicsCommandList::Close");

	// submit command list
	ID3D12CommandList* commandLists[] = { commandList.Get() };

	commandQueue->ExecuteCommandLists(1, commandLists);




	// Present
	ThrowIfFailed(swapChain->Present(1, 0), "IDXGISwapChain4::Present");



	waitForGPU();
	
}



void DX12Renderer::UpdateConstantBuffer(float elapsedSeconds)
{
	using namespace DirectX;

	XMMATRIX modelMatrix = XMMatrixRotationY(elapsedSeconds);

	XMMATRIX viewMatrix = XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -2.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	);

	float aspect = static_cast<float>(mWidth) / mHeight;

	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.0f);

	XMMATRIX MVP = modelMatrix * viewMatrix * projectionMatrix;

	TransformConstants data = {};

	XMStoreFloat4x4(&data.MVP, MVP);

	std::memcpy(mConstantBufferMapped, &data, sizeof(data));
}

void DX12Renderer::ThrowIfFailed(HRESULT hr, const char* operation)
{
	if (FAILED(hr))
	{
		std::ostringstream message;
		message << operation << " failed (HRESULT 0x" << std::uppercase
			<< std::hex << std::setw(8) << std::setfill('0')
			<< static_cast<unsigned long>(hr) << ')';
		char systemMessage[512] = {};
		if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, static_cast<DWORD>(hr), 0, systemMessage,
			static_cast<DWORD>(sizeof(systemMessage)), nullptr) != 0) {
			message << ": " << systemMessage;
		}
		throw std::runtime_error(message.str());
	}
}

void DX12Renderer::waitForGPU() {
	++fenceValue;

	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue), "ID3D12CommandQueue::Signal");
	if (fence->GetCompletedValue() < fenceValue) {
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent), "ID3D12Fence::SetEventOnCompletion");
		if (WaitForSingleObject(fenceEvent, INFINITE) != WAIT_OBJECT_0) {
			throw std::system_error(GetLastError(), std::system_category(), "WaitForSingleObject failed");
		}
	}
}
