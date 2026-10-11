#include "DX12Renderer.h"

Vertex vertices[] = {
	// Front (-Z)
	{{-.5f,-.5f,-.5f}, {1,1,1,1}, {0,1}, { 0, 0,-1}},
	{{-.5f, .5f,-.5f}, {1,1,1,1}, {0,0}, { 0, 0,-1}},
	{{ .5f, .5f,-.5f}, {1,1,1,1}, {1,0}, { 0, 0,-1}},
	{{ .5f,-.5f,-.5f}, {1,1,1,1}, {1,1}, { 0, 0,-1}},

	// Back (+Z)
	{{-.5f,-.5f, .5f}, {1,1,1,1}, {0,1}, { 0, 0, 1}},
	{{ .5f,-.5f, .5f}, {1,1,1,1}, {1,1}, { 0, 0, 1}},
	{{ .5f, .5f, .5f}, {1,1,1,1}, {1,0}, { 0, 0, 1}},
	{{-.5f, .5f, .5f}, {1,1,1,1}, {0,0}, { 0, 0, 1}},

	// Left (-X)
	{{-.5f,-.5f, .5f}, {1,1,1,1}, {0,1}, {-1, 0, 0}},
	{{-.5f, .5f, .5f}, {1,1,1,1}, {0,0}, {-1, 0, 0}},
	{{-.5f, .5f,-.5f}, {1,1,1,1}, {1,0}, {-1, 0, 0}},
	{{-.5f,-.5f,-.5f}, {1,1,1,1}, {1,1}, {-1, 0, 0}},

	// Right (+X)
	{{ .5f,-.5f,-.5f}, {1,1,1,1}, {0,1}, { 1, 0, 0}},
	{{ .5f, .5f,-.5f}, {1,1,1,1}, {0,0}, { 1, 0, 0}},
	{{ .5f, .5f, .5f}, {1,1,1,1}, {1,0}, { 1, 0, 0}},
	{{ .5f,-.5f, .5f}, {1,1,1,1}, {1,1}, { 1, 0, 0}},

	// Top (+Y)
	{{-.5f, .5f,-.5f}, {1,1,1,1}, {0,1}, { 0, 1, 0}},
	{{-.5f, .5f, .5f}, {1,1,1,1}, {0,0}, { 0, 1, 0}},
	{{ .5f, .5f, .5f}, {1,1,1,1}, {1,0}, { 0, 1, 0}},
	{{ .5f, .5f,-.5f}, {1,1,1,1}, {1,1}, { 0, 1, 0}},

	// Bottom (-Y)
	{{-.5f,-.5f, .5f}, {1,1,1,1}, {0,1}, { 0,-1, 0}},
	{{-.5f,-.5f,-.5f}, {1,1,1,1}, {0,0}, { 0,-1, 0}},
	{{ .5f,-.5f,-.5f}, {1,1,1,1}, {1,0}, { 0,-1, 0}},
	{{ .5f,-.5f, .5f}, {1,1,1,1}, {1,1}, { 0,-1, 0}}
};

uint16_t indices[] = {
	 0,  1,  2,   0,  2,  3,
	 4,  5,  6,   4,  6,  7,
	 8,  9, 10,   8, 10, 11,
	12, 13, 14,  12, 14, 15,
	16, 17, 18,  16, 18, 19,
	20, 21, 22,  20, 22, 23
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

void DX12Renderer::initialize(HWND hwnd)
{
	// 1. Device, command recording, and GPU synchronization.
	initializeDevice();
	initializeCommands();

	// 2. Window output, depth testing, and camera.
	initializeSwapChain(hwnd);
	initializeRenderTargets();
	initializeDepthBuffer();
	initializeCameraAndViewport();

	// 3. Pipeline bindings and shaders.
	initializePipeline();

	// 4. Scene data and per-frame constants.
	initializeGeometry();
	initializeConstantBuffer();
	initializeLightBuffer();

	// 5. Texture data, upload commands, and shader view.
	initializeTexture();
}

void DX12Renderer::initializeDevice()
{
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
}

void DX12Renderer::initializeCommands()
{
	HRESULT hr = S_OK;

	// -----------  Create command Queue  ---------------------

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};

	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

	ThrowIfFailed(hr, "ID3D12Device::CreateCommandQueue");

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

	// ----------- Create Fence --------
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
}

void DX12Renderer::initializeSwapChain(HWND hwnd)
{
	HRESULT hr = S_OK;

	// ----------  Create swap chain  ------------------

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

	RECT clientRect{};
	if (!GetClientRect(hwnd, &clientRect)) {
		throw std::system_error(GetLastError(), std::system_category(), "GetClientRect failed");
	}

	mWidth = clientRect.right - clientRect.left;
	mHeight = clientRect.bottom - clientRect.top;

	if (mWidth == 0 || mHeight == 0) {
		throw std::invalid_argument("The window client area must have a nonzero size.");
	}

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

	hr = factory->MakeWindowAssociation(
		hwnd,
		DXGI_MWA_NO_ALT_ENTER
	); // ban Alt + Enter as Fullscreen
	ThrowIfFailed(hr, "IDXGIFactory6::MakeWindowAssociation");
}

void DX12Renderer::initializeRenderTargets()
{
	HRESULT hr = S_OK;

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
}

void DX12Renderer::initializeDepthBuffer()
{
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

	D3D12_HEAP_PROPERTIES heapProperties = {};

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

void DX12Renderer::initializeCameraAndViewport()
{
	// set camera
	mCamera = std::make_unique<Camera>(
		XM_PIDIV4,
		static_cast<float>(mWidth) / static_cast<float>(mHeight),
		0.1f,
		100.0f,
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

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
}

void DX12Renderer::initializePipeline()
{
	createRootSignature();
	compileShaders();
	createPipelineState();
}

void DX12Renderer::createRootSignature()
{
	HRESULT hr = S_OK;

	// Create Sampler

	D3D12_STATIC_SAMPLER_DESC samplerDescription = {};

	samplerDescription.Filter =
		D3D12_FILTER_MIN_MAG_MIP_POINT;

	samplerDescription.AddressU =
		D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	samplerDescription.AddressV =
		D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	samplerDescription.AddressW =
		D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	samplerDescription.MipLODBias = 0.0f;
	samplerDescription.MaxAnisotropy = 1;

	samplerDescription.ComparisonFunc =
		D3D12_COMPARISON_FUNC_ALWAYS;

	samplerDescription.BorderColor =
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	samplerDescription.MinLOD = 0.0f;
	samplerDescription.MaxLOD = D3D12_FLOAT32_MAX;

	samplerDescription.ShaderRegister = 0;
	samplerDescription.RegisterSpace = 0;

	samplerDescription.ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;

	// ----------- Create RootSignature -------------
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_ROOT_PARAMETER transformParameter[3] = {};
	transformParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	transformParameter[0].Descriptor.ShaderRegister = 0;
	transformParameter[0].Descriptor.RegisterSpace = 0;
	transformParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Texture SRV
	D3D12_DESCRIPTOR_RANGE textureDescriptorRange = {};

	textureDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	textureDescriptorRange.NumDescriptors = 1;
	textureDescriptorRange.BaseShaderRegister = 0;
	textureDescriptorRange.RegisterSpace = 0;

	textureDescriptorRange.OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Root Parameter 1
	transformParameter[1].ParameterType =
		D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

	transformParameter[1].DescriptorTable.NumDescriptorRanges = 1;

	transformParameter[1].DescriptorTable.pDescriptorRanges =
		&textureDescriptorRange;

	transformParameter[1].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;

	// Root Parameter 2: LightConstant -> b1, pixel shader.
	transformParameter[2].ParameterType =
		D3D12_ROOT_PARAMETER_TYPE_CBV;

	transformParameter[2].Descriptor.ShaderRegister = 1;
	transformParameter[2].Descriptor.RegisterSpace = 0;

	transformParameter[2].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;

	rootSignatureDesc.NumParameters = _countof(transformParameter);
	rootSignatureDesc.pParameters = transformParameter;

	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDescription;

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
}

void DX12Renderer::compileShaders()
{
	HRESULT hr = S_OK;

	// ----------- Compile shaders ------------
	hr = D3DCompileFromFile(
		L"shader/shader.hlsl",
		nullptr,
		nullptr,
		"VSMain",
		"vs_5_0",
		0,
		0,
		&vertexShader,
		&errorBlob
	);

	ThrowIfFailed(hr, "Compile Vertex Shader");

	hr = D3DCompileFromFile(
		L"shader/shader.hlsl",
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		0,
		0,
		&pixelShader,
		&errorBlob
	);

	ThrowIfFailed(hr, "Compile Pixel Shader");
}

void DX12Renderer::createPipelineState()
{
	HRESULT hr = S_OK;

	// -------------- Input Layout ----------
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			static_cast<UINT>(offsetof(Vertex, position)),
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		{
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			static_cast<UINT>(offsetof(Vertex, color)),
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			static_cast<UINT>(offsetof(Vertex, uv)),
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			static_cast<UINT>(offsetof(Vertex, normal)),
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};

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

	psoDesc.InputLayout =
	{
		inputLayout,
		_countof(inputLayout)
	};

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
	ThrowIfFailed(hr, "Create PSO");
}

void DX12Renderer::initializeGeometry()
{
	// Vertex and index data use CPU-writable upload buffers in this learning sample.
	const UINT vertexBufferSize = sizeof(vertices);
	vertexBuffer = createUploadBuffer(vertexBufferSize);
	writeUploadBuffer(vertexBuffer.Get(), vertices, vertexBufferSize);

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vertexBufferSize;
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	const UINT indexBufferSize = sizeof(indices);
	indexBuffer = createUploadBuffer(indexBufferSize);
	writeUploadBuffer(indexBuffer.Get(), indices, indexBufferSize);

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = indexBufferSize;
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

void DX12Renderer::initializeConstantBuffer()
{
	// Constant buffer sizes must be aligned to 256 bytes.
	const UINT constantBufferSize =
		(sizeof(TransformConstants) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)
		& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
	mConstantBuffer = createUploadBuffer(constantBufferSize);

	void* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(mConstantBuffer->Map(0, &readRange, &mappedData), "Map Constant Buffer");

	// Keep this buffer mapped because UpdateConstantBuffer writes it every frame.
	mConstantBufferMapped = static_cast<uint8_t*>(mappedData);
}

void DX12Renderer::initializeTexture()
{
	const TextureData textureData = prepareTextureData();
	createTextureResource(textureData.width, textureData.height);
	createTextureView();

	beginInitializationUpload();
	// Keep the staging buffer alive until finishInitializationUpload has waited for the GPU.
	const ComPtr<ID3D12Resource> uploadBuffer = recordTextureUpload(textureData);
	finishInitializationUpload();
}

DX12Renderer::TextureData DX12Renderer::prepareTextureData() const
{
	// Editable data preparation: dimensions and RGBA8 pixels belong here.
	// Preserve the current window-sized, zero-filled texture as the default.
	TextureData data;
	data.width = mWidth;
	data.height = mHeight;
	data.rowPitch = static_cast<size_t>(data.width) * TextureBytesPerPixel;
	data.pixels.resize(data.rowPitch * data.height);
	return data;
}

void DX12Renderer::createTextureResource(UINT width, UINT height)
{
	// GPU texture: one RGBA8 mip, initially used as the copy destination.
	D3D12_RESOURCE_DESC textureDescription = {};
	textureDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDescription.Width = width;
	textureDescription.Height = height;
	textureDescription.DepthOrArraySize = 1;
	textureDescription.MipLevels = 1;
	textureDescription.Format = TextureFormat;
	textureDescription.SampleDesc.Count = 1;
	textureDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	ThrowIfFailed(
		device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureDescription,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&textureResource)
		),
		"Create Texture Resource"
	);
}

void DX12Renderer::createTextureView()
{
	// One shader-visible SRV for the texture at register t0.
	D3D12_DESCRIPTOR_HEAP_DESC heapDescription = {};
	heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDescription.NumDescriptors = 1;
	heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(
		device->CreateDescriptorHeap(&heapDescription, IID_PPV_ARGS(&shaderResourceDescriptorHeap)),
		"Create Shader Resource Descriptor Heap"
	);

	const D3D12_RESOURCE_DESC textureDescription = textureResource->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC viewDescription = {};
	viewDescription.Format = textureDescription.Format;
	viewDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDescription.Texture2D.MostDetailedMip = 0;
	viewDescription.Texture2D.MipLevels = textureDescription.MipLevels;
	device->CreateShaderResourceView(
		textureResource.Get(),
		&viewDescription,
		shaderResourceDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
	);
}

ComPtr<ID3D12Resource> DX12Renderer::recordTextureUpload(const TextureData& data)
{
	// Fixed upload procedure: query the GPU copy layout instead of guessing its row pitch.
	const D3D12_RESOURCE_DESC textureDescription = textureResource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT uploadLayout = {};
	UINT numberOfRows = 0;
	UINT64 rowSizeInBytes = 0;
	UINT64 requiredUploadBufferSize = 0;
	device->GetCopyableFootprints(
		&textureDescription, 0, 1, 0,
		&uploadLayout, &numberOfRows, &rowSizeInBytes, &requiredUploadBufferSize
	);

	ComPtr<ID3D12Resource> uploadBuffer = createUploadBuffer(requiredUploadBufferSize);
	void* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(uploadBuffer->Map(0, &readRange, &mappedData), "Map Texture Upload Buffer");

	uint8_t* destinationData = static_cast<uint8_t*>(mappedData);
	for (UINT rowIndex = 0; rowIndex < numberOfRows; ++rowIndex) {
		uint8_t* destinationRow = destinationData + uploadLayout.Offset
			+ static_cast<size_t>(rowIndex) * uploadLayout.Footprint.RowPitch;
		const uint8_t* sourceRow = data.pixels.data()
			+ static_cast<size_t>(rowIndex) * data.rowPitch;
		std::memcpy(destinationRow, sourceRow, static_cast<size_t>(rowSizeInBytes));
	}

	D3D12_RANGE writtenRange = { 0, static_cast<SIZE_T>(requiredUploadBufferSize) };
	uploadBuffer->Unmap(0, &writtenRange);

	D3D12_TEXTURE_COPY_LOCATION sourceLocation = {};
	sourceLocation.pResource = uploadBuffer.Get();
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	sourceLocation.PlacedFootprint = uploadLayout;

	D3D12_TEXTURE_COPY_LOCATION destinationLocation = {};
	destinationLocation.pResource = textureResource.Get();
	destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destinationLocation.SubresourceIndex = 0;
	commandList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);

	D3D12_RESOURCE_BARRIER textureBarrier = {};
	textureBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	textureBarrier.Transition.pResource = textureResource.Get();
	textureBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	textureBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	textureBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &textureBarrier);
	return uploadBuffer;
}

ComPtr<ID3D12Resource> DX12Renderer::createUploadBuffer(UINT64 sizeInBytes)
{
	// Shared setup for vertex, index, constant, and texture staging buffers.
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC bufferDescription = {};
	bufferDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDescription.Width = sizeInBytes;
	bufferDescription.Height = 1;
	bufferDescription.DepthOrArraySize = 1;
	bufferDescription.MipLevels = 1;
	bufferDescription.Format = DXGI_FORMAT_UNKNOWN;
	bufferDescription.SampleDesc.Count = 1;
	bufferDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ComPtr<ID3D12Resource> buffer;
	ThrowIfFailed(
		device->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDescription,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer)
		),
		"Create Upload Buffer"
	);
	return buffer;
}

void DX12Renderer::writeUploadBuffer(ID3D12Resource* buffer, const void* data, SIZE_T sizeInBytes)
{
	void* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(buffer->Map(0, &readRange, &mappedData), "Map Upload Buffer");
	std::memcpy(mappedData, data, sizeInBytes);
	D3D12_RANGE writtenRange = { 0, sizeInBytes };
	buffer->Unmap(0, &writtenRange);
}

void DX12Renderer::beginInitializationUpload()
{
	// Called during initialization, before any rendering uses this allocator.
	ThrowIfFailed(commandAllocator->Reset(), "Reset Command Allocator (initialization upload)");
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr), "Reset Command List (initialization upload)");
}

void DX12Renderer::finishInitializationUpload()
{
	ThrowIfFailed(commandList->Close(), "Close Command List (initialization upload)");
	ID3D12CommandList* uploadCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(uploadCommandLists), uploadCommandLists);
	// Signal after submission; staging resources may be released only after this wait returns.
	waitForGPU();
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

	// Bind Shader-Visible Descriptor Heap
	ID3D12DescriptorHeap* descriptorHeaps[] = {
		shaderResourceDescriptorHeap.Get()
	};

	commandList->SetDescriptorHeaps(
		1,
		descriptorHeaps
	);

	// Bind Texture SRV
	commandList->SetGraphicsRootDescriptorTable(
		1,
		shaderResourceDescriptorHeap
		->GetGPUDescriptorHandleForHeapStart()
	);




	auto now = std::chrono::steady_clock::now();

	float elapsedSeconds =
		std::chrono::duration<float>(
			now - mStartTime
		).count();

	mCamera->update();

	UpdateConstantBuffer(elapsedSeconds);
	UpdateLightBuffer();

	commandList->SetGraphicsRootConstantBufferView(2, mLightBuffer->GetGPUVirtualAddress());

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



void DX12Renderer::UpdateLightBuffer()
{
	LightConstant data = {};

	data.lightDirection = { -1.0f, -1.0f, -1.0f };

	data.diffuseIntensity = 0.8f;
	data.ambientIntensity = 0.7f;

	std::memcpy(
		mLightBufferMapped,
		&data,
		sizeof(data)
	);
}

void DX12Renderer::UpdateConstantBuffer(float elapsedSeconds)
{
	using namespace DirectX;

	XMMATRIX modelMatrix = XMMatrixRotationY(elapsedSeconds);

	XMMATRIX viewMatrix = mCamera.get()->getViewMatrix();

	float aspect = static_cast<float>(mWidth) / mHeight;

	XMMATRIX projectionMatrix = mCamera.get()->getProjectionMatrix();

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

void DX12Renderer::initializeLightBuffer() {
	static_assert(sizeof(LightConstant) == 32, "LightConstant must match the HLSL layout");

	constexpr UINT alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

	const UINT bufferSize = (static_cast<UINT>(sizeof(LightConstant)) + alignment - 1) & ~(alignment - 1);

	mLightBuffer = createUploadBuffer(bufferSize);

	D3D12_RANGE readRange = { 0 ,0 };
	void* mappedData = nullptr;

	ThrowIfFailed(mLightBuffer->Map(0, &readRange, &mappedData), "Map Light Buffer");

	mLightBufferMapped = static_cast<uint8_t*>(mappedData);

	UpdateLightBuffer();
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
