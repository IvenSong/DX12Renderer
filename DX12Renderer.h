#pragma once
#include "base.h"
#include "camera.h"

class DX12Renderer {
public:
	DX12Renderer();
	~DX12Renderer();

	void initialize(HWND hwnd);
	void render();
	InputHandler* inputHandler() noexcept { return mCamera.get(); }

private:
	// Initialization stages, called in dependency order by initialize().
	void initializeDevice();
	void initializeCommands();
	void initializeSwapChain(HWND hwnd);
	void initializeRenderTargets();
	void initializeDepthBuffer();
	void initializeCameraAndViewport();
	void initializePipeline();
	void initializeGeometry();
	void initializeConstantBuffer();
	void initializeTexture();

	// Pipeline preparation: sampler belongs to the root signature; input layout belongs to the PSO.
	void createRootSignature();
	void compileShaders();
	void createPipelineState();

	// Editable RGBA8 data is separate from resource creation and the fixed upload procedure.
	struct TextureData {
		UINT width = 0;
		UINT height = 0;
		size_t rowPitch = 0;
		std::vector<uint8_t> pixels;
	};
	TextureData prepareTextureData() const;
	void createTextureResource(UINT width, UINT height);
	void createTextureView();
	Microsoft::WRL::ComPtr<ID3D12Resource> recordTextureUpload(const TextureData& data);

	// Shared CPU-writable buffer setup and initialization command submission.
	Microsoft::WRL::ComPtr<ID3D12Resource> createUploadBuffer(UINT64 sizeInBytes);
	void writeUploadBuffer(ID3D12Resource* buffer, const void* data, SIZE_T sizeInBytes);
	void beginInitializationUpload();
	void finishInitializationUpload();

	// Per-frame work, synchronization, and error reporting.
	void UpdateConstantBuffer(float elapsedSeconds);
	void waitForGPU();
	void ThrowIfFailed(HRESULT hr, const char* operation);

private:
	// Device and adapter.
	Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	// Command recording and GPU synchronization.
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	// Swap chain, render targets, and depth testing.
	static constexpr UINT FrameCount = 2;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	UINT rtvDescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;

	// Pipeline and shader compilation.
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	// Geometry and persistently mapped per-frame constants.
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> mConstantBuffer;
	uint8_t* mConstantBufferMapped = nullptr;

	// Window dimensions, viewport, and camera.
	UINT mWidth = 0;
	UINT mHeight = 0;
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
	std::unique_ptr<Camera> mCamera;
	std::chrono::steady_clock::time_point mStartTime = std::chrono::steady_clock::now();
	const float clearColor[4] = { 0.1f, 0.4f, 0.2f, 1.0f };

	// One RGBA8 texture and its shader-visible view. Staging resources are local to initialization.
	static constexpr DXGI_FORMAT TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr UINT TextureBytesPerPixel = 4;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> shaderResourceDescriptorHeap;
};
