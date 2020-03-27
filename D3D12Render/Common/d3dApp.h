#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"
#include <Windowsx.h>

//链接需要的D3D12库
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

//单例,只允许全局有一个实例

class D3DApp
{

protected:

    D3DApp();
    D3DApp(HINSTANCE hInstance);
    //不希望D3DApp类被拷贝，故把相关拷贝构造函数与运算符标记为删除
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;
    virtual ~D3DApp();

public:

    static D3DApp* GetApp();
    HINSTANCE APPInst() const;
    HWND MainWnd() const;

    //宽高比
    float AspectRatio() const;

    bool Get4xMsaaState() const;
    void Set4xMsaaState(bool value);

    //消息循环函数
    int Run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void CreateRtvAndDsvDescriptorHeap();
    virtual void OnResize();
    virtual void Update(const GameTimer& gt) = 0;
    virtual void Draw(const GameTimer& gt) = 0;

    //便于重写鼠标输入消息的处理流程
    virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
    virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
    virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

protected:
    bool InitialMainWindow();
    bool InitialDirect3D();
    //创建命令列表，命令分配器，以及命令队列
    void CreateCommandObject();
    //创建交换链
    void CreateSwapChain();

    void FlushCommandQueue();

    //获取当前后台缓冲区
    ID3D12Resource* CurrentBackBuffer() const 
    {
        return mSwapChainBuffer[mCurrentBackBuffer].Get();
    }

    //获取当前后台缓冲区描述符
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrentBackBuffer,mRtvDescriptorSize);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const
    {
        return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    }

    void CalculateFrameStats();

    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* Output, DXGI_FORMAT format);

protected:
    static D3DApp* mApp;

    HINSTANCE mhAppInst = nullptr;              //应用程序实例句柄
    HWND mhMainWnd = nullptr;                   //主窗口句柄
    bool mAppPaused = false;                    //应用程序是否暂停
    bool mMinimized = false;                    //应用程序是否最小化
    bool mMaximized = false;                    //应用程序是否最大化
    bool mResizing = false;                     //是否正在拖动窗口调整栏
    bool mFullscreenState = false;              //是否开启全屏模式

    bool m4xMsaaState = false;                  //是否开启4X MSAA
    UINT m4xMsaaQuality = 0;                    //4X MSAA的质量级别

    GameTimer mTimer;

    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mdxgiSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    Microsoft::WRL::ComPtr<ID3D12Fence> md3dFence;
    UINT64 mCurrentFence = 0;

    //CommandObject
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

    static const int SwapChainBufferCount = 2;
    int mCurrentBackBuffer = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    //缓冲区描述符堆
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;      //渲染目标描述符堆
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;      //深度/模板缓冲区描述符堆

    //Viewport
    D3D12_VIEWPORT mScreenViewport;

    //裁剪矩形
    D3D12_RECT mScissorRect;

    //描述符大小
    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    //下列值应该在派生类的构造函数中自定义
    std::wstring mMainWindowCaption = L"d3d App";
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;             //后台缓冲区数据格式
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;        //深度/模板缓冲区数据格式
    int mClientWidth = 800;
    int mClientHeight = 600;
};