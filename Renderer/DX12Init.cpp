#include "DX12Renderer.h"

using namespace Microsoft::WRL;

void DX12Renderer::initializeDevice()
{
	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	// initialize factory
	HRESULT hr = CreateDXGIFactory2(
		dxgiFactoryFlags,
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
