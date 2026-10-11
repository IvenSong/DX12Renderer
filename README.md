# DX12Renderer

A personal 3D rendering project built with **C++17, DirectX 12, HLSL, DirectXMath, and Win32**.

## Implemented Features

- A rotating cube with indexed drawing, depth testing, and per-face normals.
- A camera supporting keyboard movement and mouse look.
- MVP transforms with basic ambient and directional diffuse shading.
- A dedicated LightBuffer passed to the pixel shader through `b1`.
- RGBA8 texture creation, upload, SRV binding, and static sampler setup.

Lighting is a work in progress: world-space normal transforms and diffuse light color/intensity controls are pending. Image loading and texture sampling are not enabled.

## Technical Highlights

- **Rendering pipeline:** Command queue/list, root signature, PSO, and a double-buffered swap chain.
- **Data layout:** DirectXMath vertex types matched to HLSL through `offsetof`; separate, persistently mapped transform (`b0`) and lighting (`b1`) buffers with 256-byte aligned allocations.
- **GPU resource management:** Explicit state transitions and fence synchronization, keeping upload buffers alive until the GPU finishes.
- **Code organization:** Initialization split into stages, shared buffer helpers, `ComPtr` ownership, and `HRESULT` error checks.

## Build and Controls

Requires Windows 10/11, Visual Studio 2022 with **Desktop development with C++**, MSVC v143, a Windows SDK, and a GPU supporting DirectX 12 Feature Level 12_0.

1. Open [DX12_Start.sln](DX12_Start.sln).
2. Select **Debug | x64** and set **DX12_Start** as the startup project.
3. Build and run with **F5** or **Ctrl+F5**.

Keep the debugging working directory set to `$(ProjectDir)` (configured by default). The application compiles `shader/shader.hlsl` at startup.

To adjust light direction and ambient intensity, edit `UpdateLightBuffer()` in [DX12Renderer.cpp](Renderer/DX12Renderer.cpp).

| Control | Action |
| --- | --- |
| W / S | Move forward / backward |
| A / D | Move sideways |
| Hold right mouse button and move | Look around |
| Close the window | Exit |
