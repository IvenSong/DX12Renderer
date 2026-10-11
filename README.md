# DX12Renderer

A personal 3D rendering project built with **C++17, DirectX 12, HLSL, DirectXMath, and Win32**.

## Implemented Features

- Two rotating cubes with indexed drawing and depth testing.
- A camera supporting keyboard movement and mouse look.
- Model, view, and projection transforms, with UV color visualization and a flipped-V comparison.
- RGBA8 texture creation, upload, SRV binding, and static sampler setup.

The current demo displays UV colors. Image loading and texture sampling are not yet enabled.

## Technical Highlights

- **Rendering pipeline:** Command queue/list, root signature, PSO, and a double-buffered swap chain.
- **Data layout:** DirectXMath vertex types matched to HLSL through `offsetof`; a persistently mapped constant buffer with 256-byte aligned allocation.
- **GPU resource management:** Explicit state transitions and fence synchronization, keeping upload buffers alive until the GPU finishes.
- **Code organization:** Initialization split into stages, shared buffer helpers, `ComPtr` ownership, and `HRESULT` error checks.

## Build and Controls

Requires Windows 10/11, Visual Studio 2022 with **Desktop development with C++**, MSVC v143, a Windows SDK, and a GPU supporting DirectX 12 Feature Level 12_0.

1. Open [DX12_Start.sln](DX12_Start.sln).
2. Select **Debug | x64** and set **DX12_Start** as the startup project.
3. Build and run with **F5** or **Ctrl+F5**.

Keep the debugging working directory set to `$(ProjectDir)` (configured by default). The application compiles `shader/shader.hlsl` at startup.

| Control | Action |
| --- | --- |
| W / S | Move forward / backward |
| A / D | Move sideways |
| Hold right mouse button and move | Look around |
| Close the window | Exit |
