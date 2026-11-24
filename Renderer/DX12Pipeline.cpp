#include "DX12Renderer.h"

using namespace Microsoft::WRL;

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

	D3D12_STATIC_SAMPLER_DESC samplerDescription[2] = {};

	// ---------- Texture Sampler -----------
	samplerDescription[0].Filter =
		D3D12_FILTER_MIN_MAG_MIP_POINT;

	samplerDescription[0].AddressU =
		D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	samplerDescription[0].AddressV =
		D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	samplerDescription[0].AddressW =
		D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	samplerDescription[0].MipLODBias = 0.0f;
	samplerDescription[0].MaxAnisotropy = 1;

	samplerDescription[0].ComparisonFunc =
		D3D12_COMPARISON_FUNC_ALWAYS;

	samplerDescription[0].BorderColor =
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	samplerDescription[0].MinLOD = 0.0f;
	samplerDescription[0].MaxLOD = D3D12_FLOAT32_MAX;

	samplerDescription[0].ShaderRegister = 0;
	samplerDescription[0].RegisterSpace = 0;

	samplerDescription[0].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;

	// ------- Shadow Sampler ---------- 
	samplerDescription[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // 2x2 PCF
	samplerDescription[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDescription[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDescription[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDescription[1].MipLODBias = 0.0f;
	samplerDescription[1].MaxAnisotropy = 1;
	samplerDescription[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDescription[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE; // ³¬³öÒõÓ°·¶Î§Ëã 1.0£¨ÁÁ×Å£©
	samplerDescription[1].MinLOD = 0.0f;
	samplerDescription[1].MaxLOD = D3D12_FLOAT32_MAX;
	samplerDescription[1].ShaderRegister = 1; // corresponding HLSL register(s1)
	samplerDescription[1].RegisterSpace = 0;
	samplerDescription[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ----------- Create RootSignature -------------
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_ROOT_PARAMETER transformParameter[RootSlotCount] = {};
	transformParameter[RootSlotTransformCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	transformParameter[RootSlotTransformCBV].Descriptor.ShaderRegister = 0;
	transformParameter[RootSlotTransformCBV].Descriptor.RegisterSpace = 0;
	transformParameter[RootSlotTransformCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Texture SRV
	D3D12_DESCRIPTOR_RANGE textureDescriptorRange = {};

	textureDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	textureDescriptorRange.NumDescriptors = 2;
	textureDescriptorRange.BaseShaderRegister = 0;
	textureDescriptorRange.RegisterSpace = 0;

	textureDescriptorRange.OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Root Parameter 1
	transformParameter[RootSlotTextureTable].ParameterType =
		D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

	transformParameter[RootSlotTextureTable].DescriptorTable.NumDescriptorRanges = 1;

	transformParameter[RootSlotTextureTable].DescriptorTable.pDescriptorRanges =
		&textureDescriptorRange;

	transformParameter[RootSlotTextureTable].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;

	// Root Parameter 2: LightConstant -> b1, pixel shader.
	transformParameter[RootSlotLightCBV].ParameterType =
		D3D12_ROOT_PARAMETER_TYPE_CBV;

	transformParameter[RootSlotLightCBV].Descriptor.ShaderRegister = 1;
	transformParameter[RootSlotLightCBV].Descriptor.RegisterSpace = 0;

	transformParameter[RootSlotLightCBV].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;

	rootSignatureDesc.NumParameters = _countof(transformParameter);
	rootSignatureDesc.pParameters = transformParameter;

	rootSignatureDesc.NumStaticSamplers = _countof(samplerDescription);
	rootSignatureDesc.pStaticSamplers = samplerDescription;

	


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
		"ShadowVSMain",
		"vs_5_0",
		0,
		0,
		&mShadowVertexShader,
		&errorBlob
	);

	ThrowIfFailed(hr, "Compile Shadow Map Vertex Shader");

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

D3D12_GRAPHICS_PIPELINE_STATE_DESC
DX12Renderer::createDefaultPSODesc() noexcept
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

	// =================================================
	// 1. Rasterizer State
	// Controls how primitives are rasterized.
	// =================================================

	auto& rs = desc.RasterizerState;

	// Fill triangles with solid pixels instead of wireframes.
	rs.FillMode = D3D12_FILL_MODE_SOLID;

	// Cull back-facing triangles.
	rs.CullMode = D3D12_CULL_MODE_BACK;

	// FALSE: Clockwise winding order defines front-facing triangles.
	rs.FrontCounterClockwise = FALSE;

	// Depth bias settings (may need adjustment for Shadow Mapping).
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0.0f;
	rs.SlopeScaledDepthBias = 0.0f;

	// Enable depth clipping.
	rs.DepthClipEnable = TRUE;

	// Disable special multisample and line anti-aliasing behavior.
	rs.MultisampleEnable = FALSE;
	rs.AntialiasedLineEnable = FALSE;

	// Do not force a specific sample count.
	rs.ForcedSampleCount = 0;

	// Disable conservative rasterization.
	rs.ConservativeRaster =
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


	// =================================================
	// 2. Blend State
	// Controls how pixel shader outputs are blended
	// with existing render target contents.
	// =================================================

	auto& blend = desc.BlendState;

	// Disable alpha-to-coverage.
	blend.AlphaToCoverageEnable = FALSE;

	// Use the same blend settings for all render targets.
	blend.IndependentBlendEnable = FALSE;

	D3D12_RENDER_TARGET_BLEND_DESC rtBlend = {};

	// Disable color blending.
	rtBlend.BlendEnable = FALSE;

	// Disable logical operations.
	rtBlend.LogicOpEnable = FALSE;

	// Color: Source * 1 + Destination * 0.
	rtBlend.SrcBlend = D3D12_BLEND_ONE;
	rtBlend.DestBlend = D3D12_BLEND_ZERO;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;

	// Alpha: Source * 1 + Destination * 0.
	rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;

	// No logical operation.
	rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;

	// Enable writing to all RGBA channels.
	rtBlend.RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	// Initialize valid blend settings for all render target slots.
	for (auto& target : blend.RenderTarget)
	{
		target = rtBlend;
	}


	// =================================================
	// 3. Depth / Stencil State
	// Controls depth testing, depth writing,
	// and stencil operations.
	// =================================================

	auto& ds = desc.DepthStencilState;

	// Enable depth testing.
	ds.DepthEnable = TRUE;

	// Allow writing depth values to the depth buffer.
	ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	// Pass the depth test if the incoming depth is smaller.
	ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// Disable stencil testing.
	ds.StencilEnable = FALSE;

	// Default stencil read and write masks.
	ds.StencilReadMask =
		D3D12_DEFAULT_STENCIL_READ_MASK;

	ds.StencilWriteMask =
		D3D12_DEFAULT_STENCIL_WRITE_MASK;

	// Keep existing stencil values for all outcomes.
	D3D12_DEPTH_STENCILOP_DESC stencilOp = {};

	stencilOp.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	stencilOp.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	stencilOp.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	stencilOp.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Apply the same stencil operations to both faces.
	ds.FrontFace = stencilOp;
	ds.BackFace = stencilOp;


	// =================================================
	// 4. Input Assembly / Multisampling
	// =================================================

	// Enable all sample bits.
	desc.SampleMask = UINT_MAX;

	// Use a single sample per pixel (MSAA disabled).
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	// The pipeline processes triangle primitives.
	desc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Disable primitive restart for index buffer strips.
	desc.IBStripCutValue =
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;


	// =================================================
	// 5. Additional Pipeline Settings
	// =================================================

	// Use the default GPU node.
	desc.NodeMask = 0;

	// Do not use cached pipeline state data.
	desc.CachedPSO = {};

	// No additional pipeline state flags.
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;


	// =================================================
	// 6. Pass-Specific Settings
	//
	// The following fields must be configured by
	// the individual render pass when required:
	//
	// - pRootSignature
	// - VS / PS
	// - InputLayout
	// - NumRenderTargets
	// - RTVFormats
	// - DSVFormat
	//
	// Unused fields remain zero-initialized.
	// =================================================

	return desc;
}
