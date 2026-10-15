# DX12Renderer

A personal 3D rendering project built with **C++17, DirectX 12, HLSL, DirectXMath, and Win32**. The current demo renders a rotating textured cube with ambient and directional diffuse lighting and an interactive camera.

![Textured cube with ambient and directional diffuse lighting](assets/image-20261015092727750.png)

## Implemented Features

- A rotating cube with indexed drawing, depth testing, per-face normals, and UV coordinates.
- A camera supporting keyboard movement and mouse look.
- MVP transforms and world-space normals with adjustable ambient and directional diffuse lighting.
- PNG/JPEG loading from files or encoded memory through an image loader adapted from SoftRenderer, using its bundled **stb_image v2.30**.
- RGBA8 texture upload, SRV binding at `t0`, and a static point sampler with wrap addressing at `s0`.
- Pixel shader texture sampling combined with vertex tint and lighting.

The default texture is `assets/textures/house_atlas.png`, copied from SoftRenderer's demo assets.

## Technical Highlights

- **Rendering pipeline:** Command queue/list, root signature, PSO, and a double-buffered swap chain.
- **Data layout:** DirectXMath vertex types matched to HLSL through `offsetof`; separate, persistently mapped transform (`b0`) and lighting (`b1`) buffers with 256-byte aligned allocations.
- **GPU resource management:** Explicit state transitions and fence synchronization, keeping upload buffers alive until the GPU finishes.
- **Code organization:** Initialization split into stages, shared buffer helpers, `ComPtr` ownership, and `HRESULT` error checks.

## Build and Run

Requires Windows 10/11, Visual Studio 2022 with **Desktop development with C++**, MSVC v143, a Windows SDK, and a GPU supporting DirectX 12 Feature Level 12_0.

1. Open [DX12Renderer.sln](DX12Renderer.sln).
2. Select **Debug | x64** and set **DX12Renderer** as the startup project.
3. Build and run with **F5** or **Ctrl+F5**.

Keep the debugger's working directory set to `$(ProjectDir)` (configured in the project). The application compiles `shader/shader.hlsl` and loads the default texture at startup. The build also copies both files to the output directory, allowing the executable to run from that directory.

## Controls

| Control | Action |
| --- | --- |
| W / S | Move forward / backward |
| A / D | Move sideways |
| Hold right mouse button and move | Look around |
| Close the window | Exit |

## Image Loading

To use another texture, place the image in `assets/textures/` and change the path in `DX12Renderer::prepareTextureData()` in [DX12Renderer.cpp](Renderer/DX12Renderer.cpp). Paths are relative to the working directory and use UTF-8. Update `CopyRuntimeAssets` in [DX12Renderer.vcxproj](DX12Renderer.vcxproj) as well to include the replacement image when running from the output directory.

[image.h](Application/image.h) provides `image::createImage(path)` and `image::createImageFromMemory(encodedData, byteCount, sourceName)`. The memory overload accepts encoded image bytes, such as the contents of a PNG file. Both return an image by value whose `mData` vector owns its pixels, so no `destroyImage()` call is needed.

Pixels use tightly packed **R, G, B, A** bytes with the first row at the **top**, matching the cube's UV layout. The texture uses the image's dimensions independently of the window size, and the upload code handles GPU row alignment. Missing or invalid images report the source name and decoding error.

The pixel shader currently uses the image's RGB and preserves vertex alpha for opaque rendering. Image alpha is available in the loaded data but is not used for transparency.

To adjust light direction, color, and ambient/diffuse intensity, edit `UpdateLightBuffer()` in [DX12Renderer.cpp](Renderer/DX12Renderer.cpp).
