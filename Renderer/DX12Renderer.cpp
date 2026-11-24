#include "DX12Renderer.h"

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

	// 6. Create Shadow Pass
	createShadowPass();
}

void DX12Renderer::createPipelineState()
{
	HRESULT hr = S_OK;



	// ---------- PSO ------------

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = createDefaultPSODesc();

	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	psoDesc.PS = { pixelShader->GetBufferPointer() , pixelShader->GetBufferSize() };

	// Keep the main pass's existing rasterization behavior; other passes use the defaults.
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.DepthClipEnable = FALSE;

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
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



void DX12Renderer::createShadowPass() {
	// Create DSV

	HRESULT hr = S_OK;

	D3D12_RESOURCE_DESC resourceDesc = {};

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;

	resourceDesc.Width = 1024;
	resourceDesc.Height = 1024;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;

	resourceDesc.Format = DXGI_FORMAT_R32_TYPELESS;


	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;

	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperty = {};

	heapProperty.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE clearValue = {};

	clearValue.Format = DXGI_FORMAT_D32_FLOAT;

	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	hr = device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&mShadowMap));

	ThrowIfFailed(hr, "Create shadow map resource");

	D3D12_DESCRIPTOR_HEAP_DESC shadowMapDesc = {};

	shadowMapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	shadowMapDesc.NumDescriptors = 1;
	shadowMapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	shadowMapDesc.NodeMask = 0;

	ThrowIfFailed(
		device->CreateDescriptorHeap(&shadowMapDesc, IID_PPV_ARGS(&mShadowMapDsvHeap)),
		"Create Shadow DSV Heap"
	);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mShadowMapDsvHeap->GetCPUDescriptorHandleForHeapStart());

	// SRV of Shadow Pass
	UINT srvDescriporSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE shadowSrvHandle = shaderResourceDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	shadowSrvHandle.ptr += srvDescriporSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, shadowSrvHandle);


	// Create PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = createDefaultPSODesc();
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};

	psoDesc.VS = { mShadowVertexShader->GetBufferPointer(), mShadowVertexShader->GetBufferSize()};

	psoDesc.NumRenderTargets = 0;

	psoDesc.pRootSignature = rootSignature.Get();

	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mShadowMapPipelineState));
	ThrowIfFailed(hr, "Create Shadow PSO");


}

void DX12Renderer::render() {
	UINT frameIndex = swapChain->GetCurrentBackBufferIndex();

	ThrowIfFailed(commandAllocator->Reset(), "ID3D12CommandAllocator::Reset");
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr), "ID3D12GraphicsCommandList::Reset");


	auto now = std::chrono::steady_clock::now();

	float elapsedSeconds =
		std::chrono::duration<float>(
			now - mStartTime
		).count();

	mCamera->update();

	UpdateConstantBuffer(elapsedSeconds);
	UpdateLightBuffer();




	renderShadowPass();


	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = mShadowMap.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	commandList->ResourceBarrier(1, &barrier);

	renderMainPass(frameIndex);


	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
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

	data.lightDirection = { -1.0f, -1.0f, 0.0f };
	data.lightColor = { 1.0f, 1.0f,1.0f };

	data.diffuseIntensity = 0.8f;
	data.ambientIntensity = 0.2f;

	std::memcpy(
		mLightBufferMapped,
		&data,
		sizeof(data)
	);
}

void DX12Renderer::UpdateConstantBuffer(float elapsedSeconds)
{
	using namespace DirectX;

	XMMATRIX viewMatrix = mCamera.get()->getViewMatrix();
	XMMATRIX projectionMatrix = mCamera.get()->getProjectionMatrix();

	XMVECTOR lightDir = XMVector3Normalize(XMVectorSet(-1.0f, -1.0f, 0.0f, 0.0f));

	// 光源虚拟相机盯住场景中心，沿着光线反方向退 10 个单位架机位
	XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR lightPos = target - lightDir * 10.0f;
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX lightView = XMMatrixLookAtRH(lightPos, target, up);
	XMMATRIX lightProj = XMMatrixOrthographicRH(10.0f, 10.0f, 0.1f, 25.0f);

	XMMATRIX lightViewProj = lightView * lightProj;

	XMMATRIX viewProj = viewMatrix * projectionMatrix;

	auto writeSlot = [&](UINT slotIndex, XMMATRIX modelMatrix) {
		TransformConstants data = {};
		XMStoreFloat4x4(&data.MVP, modelMatrix * viewProj);
		XMStoreFloat4x4(&data.normalMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, modelMatrix)));
		XMStoreFloat4x4(&data.lightMVP, modelMatrix * lightViewProj);
		std::memcpy(mConstantBufferMapped + slotIndex * 256, &data, sizeof(data));
		};

	writeSlot(0, XMMatrixRotationY(elapsedSeconds));

	writeSlot(1, XMMatrixIdentity());

	

}


void DX12Renderer::renderMainPass(UINT frameIndex) {

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

	// ---------- Input Assembler ----------
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

	commandList->SetGraphicsRootConstantBufferView(RootSlotLightCBV, mLightBuffer->GetGPUVirtualAddress());

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
		RootSlotTextureTable,
		shaderResourceDescriptorHeap
		->GetGPUDescriptorHandleForHeapStart()
	);

	const D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mConstantBuffer->GetGPUVirtualAddress();

	commandList->SetGraphicsRootConstantBufferView(RootSlotTransformCBV, cbAddress);
	commandList->DrawIndexedInstanced(
		CubeIndexCount,
		1,
		0,
		0,
		0
	);
	commandList->SetGraphicsRootConstantBufferView(RootSlotTransformCBV, cbAddress + 256);
	commandList->DrawIndexedInstanced(
		GroundIndexCount,
		1,
		GroundStartIndex,
		0,
		0
	);

	// Transition back only after all rendering commands.
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);
}


void DX12Renderer::renderShadowPass()
{


	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mShadowMapDsvHeap->GetCPUDescriptorHandleForHeapStart();

	// Output Merger
	commandList->OMSetRenderTargets(
		0,
		nullptr,
		FALSE,
		&dsvHandle
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
	D3D12_VIEWPORT shadowViewPort = { 0.0f, 0.0f, 1024.0f, 1024.0f, 0.0f, 1.0f };
	commandList->RSSetViewports(1, &shadowViewPort);

	// ---------- Scissor Rect ----------
	D3D12_RECT shadowScissor = { 0, 0, 1024, 1024 };
	commandList->RSSetScissorRects(1, &shadowScissor);

	// ---------- Input Assembler ----------
	commandList->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);

	commandList->IASetVertexBuffers(
		0,
		1,
		&vertexBufferView
	);

	commandList->IASetIndexBuffer(&indexBufferView); // Index Buffer / EBO

	commandList->SetPipelineState(mShadowMapPipelineState.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	const D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mConstantBuffer->GetGPUVirtualAddress();

	commandList->SetGraphicsRootConstantBufferView(RootSlotTransformCBV, cbAddress);
	commandList->DrawIndexedInstanced(
		CubeIndexCount,
		1,
		0,
		0,
		0
	);
	commandList->SetGraphicsRootConstantBufferView(RootSlotTransformCBV, cbAddress + 256);
	commandList->DrawIndexedInstanced(
		GroundIndexCount,
		1,
		GroundStartIndex,
		0,
		0
	);
}