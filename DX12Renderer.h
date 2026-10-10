#pragma once
#include "base.h"



class DX12Renderer {
public:

	DX12Renderer();
	~DX12Renderer();

	void initialize(HWND hwnd);

	void render();



private:
	void UpdateConstantBuffer(float elapsedSeconds);

	void ThrowIfFailed(HRESULT hr, const char* operation);

	void waitForGPU();

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;

	static constexpr UINT FrameCount = 2;
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	UINT rtvDescriptorSize = 0;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;


	// Constant buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> mConstantBuffer;
	uint8_t* mConstantBufferMapped = nullptr;

	// Time
	std::chrono::steady_clock::time_point mStartTime = std::chrono::steady_clock::now();

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};


	// Depth Test
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;


	UINT mWidth = 0;
	UINT mHeight = 0;
	const float clearColor[4] = {
		0.1f,
		0.4f,
		0.2f,
		1.0f
	};
};
