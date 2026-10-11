#include "DX12Renderer.h"


#pragma comment(linker, "/subsystem:console /entry:wWinMainCRTStartup")


LRESULT CALLBACK WindowProc(  // Deal with Events
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    auto* input = reinterpret_cast<InputHandler*>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (input)
            input->onKeyDown(static_cast<uint32_t>(wParam));
        return 0;

    case WM_KEYUP:
        if (input)
            input->onKeyUp(static_cast<uint32_t>(wParam));
        return 0;

    case WM_RBUTTONDOWN:
        if (input) {
            SetCapture(hwnd);
            input->onRMouseDown(
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        return 0;

    case WM_RBUTTONUP:
        if (input) {
            input->onRMouseUp(
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        if (GetCapture() == hwnd)
            ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (input)
            input->onMouseMove(
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    }


    return DefWindowProc(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow)
{
    try {
    // 1. 定义窗口类
    const wchar_t CLASS_NAME[] = L"DX12WindowClass";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    //



    // 2. 注册窗口类
    if (RegisterClass(&wc) == 0) {
        throw std::system_error(GetLastError(), std::system_category(), "RegisterClass failed");
    }

    // 3. 创建窗口
    HWND hwnd = CreateWindowEx(
        0,                          // Extended style
        CLASS_NAME,                 // Window class
        L"DX12 Learning",           // Window title
        WS_OVERLAPPEDWINDOW,        // Window style

        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,

        nullptr,                    // Parent window
        nullptr,                    // Menu
        hInstance,
        nullptr
    );

    if (hwnd == nullptr)
    {
        throw std::system_error(GetLastError(), std::system_category(), "CreateWindowEx failed");
    }





    // message
    MSG msg = {};

    DX12Renderer renderer;
    renderer.initialize(hwnd);

    // send message to renderer
    SetWindowLongPtr(
        hwnd,
        GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(renderer.inputHandler()));

    ShowWindow(hwnd, nCmdShow);

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(
            &msg,
            nullptr,
            0,
            0,
            PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            renderer.render();
        }
    }
    
    return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "DX12 error: " << error.what() << std::endl;
        OutputDebugStringA("DX12 error: ");
        OutputDebugStringA(error.what());
        OutputDebugStringA("\n");
        return 1;
    }
}

