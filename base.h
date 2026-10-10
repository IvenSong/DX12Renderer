#pragma once

// Windows platform definitions.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// DirectX 12, DXGI, shader compilation, and COM ownership.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>


// Standard library dependencies used throughout the project.
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

// DirectX link dependencies for the MSVC build.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")



#include ".\MathLib\mathLib.h"






struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};



struct TransformConstants {
    DirectX::XMFLOAT4X4 MVP;
};