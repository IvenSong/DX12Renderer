# DX12Renderer

使用 C++17、Win32 和 DirectX 12 编写的学习项目，逐步实现渲染初始化、三维几何、深度测试、相机输入和纹理上传。

当前场景包含两个绕 Y 轴旋转的立方体。像素着色器把 UV 坐标映射为颜色，用于观察顶点数据经过输入布局、顶点着色器和插值后是否正确传递。

## 当前功能

- 创建 DXGI 工厂、硬件适配器和 D3D12 设备。
- 使用双缓冲交换链、RTV 和 DSV 完成绘制及深度测试。
- 使用顶点缓冲、索引缓冲和 MVP 常量缓冲绘制三维场景。
- 使用 DirectXMath 的 `XMFLOAT` 类型保存顶点数据，使用 `XMVECTOR` / `XMMATRIX` 进行相机和矩阵计算。
- 支持 WASD 移动和按住鼠标右键转动视角。
- 将初始化拆分为阶段函数，并复用上传缓冲的创建和写入流程。
- 创建 RGBA8 纹理、SRV 和静态采样器，完成纹理复制、资源状态转换及 fence 同步。

**当前画面使用 UV 调试颜色，尚未切换为纹理采样。** 默认纹理数据为零，上传流程完成并不意味着已经加载了图片。

## 构建与运行

### 环境

- Windows 10 / Windows 11。
- Visual Studio 2022，安装“使用 C++ 的桌面开发”工作负载。
- MSVC v143 工具集、Windows 10/11 SDK。
- 支持 DirectX 12 Feature Level 12_0 的硬件适配器。当前代码跳过软件适配器，没有 WARP 回退路径。

项目使用 Windows SDK 提供的 DirectX 12、DXGI、D3DCompiler 和 DirectXMath，不需要另外下载第三方数学库。

### 使用 Visual Studio

1. 打开仓库根目录的 `DX12_Start.sln`。
2. 选择 `Debug | x64`，并将 `DX12_Start` 设为启动项目。
3. 确认“项目属性 → 调试 → 工作目录”为 `$(ProjectDir)`。
4. 编译后按 F5 或 Ctrl+F5 运行。

### 使用命令行

在 Visual Studio 的 Developer PowerShell / Developer Command Prompt 中进入仓库根目录：

```powershell
msbuild .\DX12_Start.vcxproj /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\DX12_Start.exe
```

`shader.hlsl` 在程序启动时通过 `D3DCompileFromFile` 编译，工程中将它排除在离线 shader 构建之外。因此运行时的工作目录必须能找到该文件；以上命令应在仓库根目录执行。

## 操作方式

| 操作 | 功能 |
| --- | --- |
| W / S | 沿相机朝向前进 / 后退 |
| A / D | 向左 / 向右移动 |
| 按住鼠标右键并移动 | 转动相机视角 |
| 关闭窗口 | 退出程序 |

## 代码组织

| 文件 | 职责 |
| --- | --- |
| `main.cpp` | 创建 Win32 窗口，处理消息循环，将键盘和鼠标事件交给输入接口 |
| `DX12Renderer.h` / `DX12Renderer.cpp` | 管理 GPU 资源、初始化阶段、纹理上传、每帧绘制和同步 |
| `base.h` | 公共依赖、`Vertex` 和 `TransformConstants` 数据结构 |
| `camera.h` / `camera.cpp` | 相机姿态、移动、视图矩阵和投影矩阵 |
| `inputHandler.h` / `inputKey.h` | 输入事件接口及按键定义 |
| `shader.hlsl` | 顶点变换、UV 传递和像素颜色输出 |
| `shaders.h` / `shaders.cpp` | 预留文件，目前 shader 编译仍在渲染器中完成 |
| `DX12_Start.sln` / `DX12_Start.vcxproj` / `.filters` | Visual Studio 解决方案、工程和文件分组 |

### 初始化阶段

`initialize(HWND)` 负责按依赖顺序调用以下阶段：

| 顺序 | 函数 | 内容 |
| --- | --- | --- |
| 1 | `initializeDevice()` | 创建工厂、选择适配器并创建设备 |
| 2 | `initializeCommands()` | 创建队列、命令分配器、命令列表、fence 和事件 |
| 3 | `initializeSwapChain(hwnd)` | 获取客户区尺寸并创建交换链 |
| 4 | `initializeRenderTargets()` | 创建 RTV 堆，获取并描述后备缓冲 |
| 5 | `initializeDepthBuffer()` | 创建 DSV 堆、深度资源和深度视图 |
| 6 | `initializeCameraAndViewport()` | 初始化相机、视口和裁剪区域 |
| 7 | `initializePipeline()` | 创建根签名、编译 shader、创建 PSO |
| 8 | `initializeGeometry()` | 创建并填充顶点和索引缓冲 |
| 9 | `initializeConstantBuffer()` | 创建并持续映射 MVP 常量缓冲 |
| 10 | `initializeTexture()` | 准备纹理数据、创建纹理及 SRV、执行上传 |

静态采样器归属于根签名的创建流程；输入布局归属于 PSO 的创建流程。`createUploadBuffer()` 与 `writeUploadBuffer()` 统一处理可供 CPU 写入的缓冲。

### 顶点数据与 shader 输入

```cpp
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 uv;
};
```

| 字段 | HLSL 类型 / 语义 | 输入格式 | 字节偏移 |
| --- | --- | --- | --- |
| `position` | `float3 / POSITION` | `R32G32B32_FLOAT` | 0 |
| `color` | `float4 / COLOR` | `R32G32B32A32_FLOAT` | 12 |
| `uv` | `float2 / TEXCOORD0` | `R32G32_FLOAT` | 28 |

当前顶点大小为 36 字节。代码通过 `offsetof(Vertex, ...)` 获取偏移，通过 `sizeof(Vertex)` 设置顶点步长。

第一个立方体使用标准 UV，第二个立方体翻转 V。当前像素着色器输出：

```hlsl
return float4(input.uv, 1.0f, 1.0f);
```

即红色由 U 控制，绿色由 V 控制，蓝色和 Alpha 固定为 1。修改 `vertices` 中各顶点最后一组坐标即可测试 UV 的传递。

### 常量与资源绑定

CPU 每帧计算 `MVP = model * view * projection`，写入持续映射的 `TransformConstants`。HLSL 使用 `row_major float4x4 MVP`，并按行向量方式执行 `mul(float4(position, 1), MVP)`。

| 资源 | shader 寄存器 | 根签名位置 |
| --- | --- | --- |
| `TransformConstants` / MVP | `b0`，顶点着色器可见 | 根参数 0，root CBV |
| `diffuseTexture` | `t0`，像素着色器可见 | 根参数 1，SRV 描述符表 |
| `textureSampler` | `s0`，像素着色器可见 | 静态采样器 |

根参数索引和 shader 寄存器编号是不同概念。当前 shader 中的 `CameraConstants` 尚未使用，也未创建独立的相机常量缓冲；实际变换只使用包含相机矩阵结果的 MVP。`outputTexture` 同样是未使用声明，没有对应的 UAV 输出流程。

### 纹理上传

可修改的数据集中在 `prepareTextureData()`：纹理宽高、CPU 行跨度和 RGBA8 像素数组。目前默认使用窗口客户区尺寸和零填充数据。

上传流程为：

1. 创建处于 `COPY_DEST` 状态的 DEFAULT 堆纹理及其 SRV。
2. 重置初始化命令列表，进入录制状态。
3. 用 `GetCopyableFootprints()` 查询布局，并创建 UPLOAD 堆缓冲。
4. `Map()` 后按目标 `RowPitch` 逐行复制 CPU 数据，再 `Unmap()`。
5. 记录 `CopyTextureRegion()`，将纹理转换为 `PIXEL_SHADER_RESOURCE`。
6. 关闭并提交命令列表，在队列上发出 fence 信号，等待 GPU 完成。
7. 初始化函数返回后，局部上传缓冲自动释放；纹理和 SRV 继续保留用于绘制。

以后测试纹理采样，需要先准备可辨识的像素数据，再让像素着色器使用 `diffuseTexture.Sample(textureSampler, input.uv)`。

## 当前限制

- 纹理流程只处理单层、单 mip 的 RGBA8 二维纹理，没有图片文件加载和 mipmap 生成。
- 每帧提交后都会等待 GPU，便于学习同步；尚未实现多帧并行的 frame resources。
- 窗口尺寸改变后尚未重建交换链、深度资源及相机投影。
- 每个立方体使用 8 个共享顶点，尚未为每个面建立独立的顶点和 UV。
- 相机移动速度按帧计算，尚未按帧间隔缩放。

## Git 提交范围

仓库保留源码、HLSL、Visual Studio 工程文件、README 和 `.gitignore`。旧的自定义 `MathLib` 已移除，数学运算使用 DirectXMath。

以下内容由 `.gitignore` 排除：

- `.vs/`、IntelliSense 数据库和其他 IDE 缓存。
- `*.vcxproj.user`、`*.suo` 等个人配置。
- `Debug/`、`Release/`、`x64/` 等构建输出目录。
- 可执行文件、对象文件、调试符号、编译后的 shader 和 MSBuild 中间文件。
- 本地环境变量文件、临时文件和日志。

本项目的 Git 根目录是包含 `DX12_Start.sln` 的文件夹；上级学习目录和其他示例项目不属于此仓库。
