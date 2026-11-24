#include "DX12Renderer.h"

using namespace Microsoft::WRL;

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
