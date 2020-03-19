#pragma once

#include <windows.h>

//主窗口句柄
HWND ghMainWnd = 0;

//初始化
bool InitWindowsApp(HINSTANCE instanceHandle, int show);


int Run();


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//win32程序中入口点为WinMain函数
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    //初始化，创建主窗口
    if (!InitWindowsApp(hInstance, nShowCmd))
    {
        return 0;
    }
    
    //初始化成功，则调用消息循环函数
    return Run();
}

bool InitWindowsApp(HINSTANCE instanceHandle, int show)
{
    //首先，创建主窗口，填写WNDCLASS结构体，在其中填写主窗口的特征
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instanceHandle;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"BasicWndClass";

    //在Windows系统中为wc注册实例
    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass FAILED", 0, 0);
        return false;
    }

    //调用CreateWindow创建窗口，此函数返回窗口句柄
    ghMainWnd = CreateWindow(
        L"BasicWndClass",       //创建此窗口采用的WNDCLASS实例名
        L"Hello Win32",         //窗口标题
        WS_OVERLAPPEDWINDOW,    //窗口的样式标志
        CW_USEDEFAULT,          //窗口左上角x坐标
        CW_USEDEFAULT,          //窗口左上角y坐标
        CW_USEDEFAULT,          //窗口高度
        CW_USEDEFAULT,          //窗口高度
        0,                      //父窗口
        0,                      //菜单句柄
        instanceHandle,         //应用程序实例句柄
        0);                     //其他参数

    if (ghMainWnd == 0)
    {
        MessageBox(0, L"CreateWindow FAILED", 0, 0);
        return false;
    }

    //显示并更新窗口
    ShowWindow(ghMainWnd, show);
    UpdateWindow(ghMainWnd);

    return true;
}

int Run()
{
    //消息队列
    MSG msg = { 0 };
    BOOL bRet = 1;

    //GetMessage函数在收到WM_QUIT消息时会返回0，此时让消息循环终止
    while ((bRet = GetMessage(&msg,0,0,0)) != 0)
    {
        //若发生错误，GetMessage函数会返回-1，让程序弹出相应消息窗口，并终止消息循环
        if (bRet == -1)
        {
            MessageBox(0, L"GetMessage FAILED", L"Error", MB_OK);
            break;
        }
        //未收到消息时，GetMessage函数令此应用程序进入休眠状态
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //与窗口相关联的消息过程在窗口创建前就在相应的WNDCLASS中写好
    switch (msg)
    {
    //按下鼠标左键时，弹出一个Hello World消息窗口
    case WM_LBUTTONDOWN:
        MessageBox(0, L"Hello, World", L"Hello", MB_OK);
        return 0;
    //收到键盘按键消息时，检测是否是ESC键，若是，则销毁主窗口(发送销毁信息)
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(ghMainWnd);
        }
        return 0;
    //收到了销毁信息，发送退出消息
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    //其余没有处理的消息转发给默认的窗口过程
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
