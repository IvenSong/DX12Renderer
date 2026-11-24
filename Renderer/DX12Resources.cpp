#include "DX12Renderer.h"
#include "Application/image.h"
#include <utility>

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
	{{ .5f,-.5f, .5f}, {1,1,1,1}, {1,1}, { 0,-1, 0}},

	// Ground (+Y)
	{{-3.f,-1.f,-3.f}, {1,1,1,1}, {0,1}, { 0, 1, 0}},
	{{-3.f,-1.f, 3.f}, {1,1,1,1}, {0,0}, { 0, 1, 0}},
	{{ 3.f,-1.f, 3.f}, {1,1,1,1}, {1,0}, { 0, 1, 0}},
	{{ 3.f,-1.f,-3.f}, {1,1,1,1}, {1,1}, { 0, 1, 0}}
};

static uint16_t indices[] = {
	 0,  1,  2,   0,  2,  3,
	 4,  5,  6,   4,  6,  7,
	 8,  9, 10,   8, 10, 11,
	12, 13, 14,  12, 14, 15,
	16, 17, 18,  16, 18, 19,
	20, 21, 22,  20, 22, 23,

	// Ground
	24, 25, 26,  24, 26, 27
};

using namespace Microsoft::WRL;

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
	const UINT slotSize =
		(sizeof(TransformConstants) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)
		& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
	static_assert(sizeof(TransformConstants) <= 256, "TransformConstants exceeds 256-byte slot");
	// slot 0 for cube£¬slot 1 for ground
	mConstantBuffer = createUploadBuffer(slotSize * 2);

	void* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(mConstantBuffer->Map(0, &readRange, &mappedData), "Map Constant Buffer");

	// Keep this buffer mapped because UpdateConstantBuffer writes it every frame.
	mConstantBufferMapped = static_cast<uint8_t*>(mappedData);
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
	// Change this path to use another image. Loading failures report the path and reason.
	image loadedImage = image::createImage("assets/textures/house_atlas.png");
	if (loadedImage.mWidth > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION
		|| loadedImage.mHeight > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
		throw std::runtime_error("Texture dimensions exceed the DirectX 12 Texture2D limit");
	}

	TextureData data;
	data.width = loadedImage.mWidth;
	data.height = loadedImage.mHeight;
	data.rowPitch = loadedImage.rowPitch();
	data.pixels = std::move(loadedImage.mData);
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
	heapDescription.NumDescriptors = 2;
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
