
#include "d3dApp.h"

D3DApp* D3DApp::mApp = nullptr;
D3DApp::D3DApp(HINSTANCE hInstance) :mhAppInst(hInstance)
{
    assert(mApp == nullptr);
    mApp = this;
}

//D3DApp::D3DApp()
//{
//    assert(mApp == nullptr);
//    mApp = this;
//}

D3DApp::~D3DApp()
{
    if (md3dDevice != nullptr)
    {
        FlushCommandQueue();
    }
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitialMainWindow()
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = mhAppInst;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"MainWindow";

    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"Register class Failed", 0, 0);
        return false;
    }

    RECT R = { 0,0,mClientWidth,mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    mhMainWnd = CreateWindow(L"MainWindow", mMainWindowCaption.c_str(), WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, width, height, 
                            0, 0, mhAppInst, 0);

    if (mhMainWnd == nullptr)
    {
        MessageBox(0, L"Create MainWindow Failed", 0, 0);
        return false;
    }

    ShowWindow(mhMainWnd, SW_SHOW);
    UpdateWindow(mhMainWnd);

    return true;
}

bool D3DApp::InitialDirect3D()
{
    #if defined(DEBUG) || defined(_DEBUG)
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
    #endif
    
    //创建IDXGIFactory
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

    //尝试创建D3DDevice
    HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));

    //若创建失败，回退至WARP设备
    if (FAILED(hardwareResult))
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice)));
    }

    //创建Fence
    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&md3dFence)));
    //获取相应描述符大小
    mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //检测对4X MSAA的支持
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = mBackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;

    ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(m4xMsaaQuality > 0 && "Unexcepted MSAA quality level.");

#ifdef _DEBUG 
    LogAdapters();
#endif

    CreateCommandObject();

    CreateSwapChain();

    CreateRtvAndDsvDescriptorHeap();

    return true;

}

void D3DApp::CreateCommandObject()
{
    D3D12_COMMAND_QUEUE_DESC mCommandQueueDesc = {}; //这里必须用花括号赋值
    mCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    mCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&mCommandQueueDesc, IID_PPV_ARGS(&mCommandQueue)));
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mDirectCmdListAlloc)));
    ThrowIfFailed(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
    ThrowIfFailed(mCommandList->Close());
}

void D3DApp::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC mSwapChainDesc;
    mSwapChainDesc.BufferDesc.Height = mClientHeight;
    mSwapChainDesc.BufferDesc.Width = mClientWidth;
    mSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    mSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    mSwapChainDesc.BufferDesc.Format = mBackBufferFormat;
    mSwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    mSwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    mSwapChainDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    mSwapChainDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    mSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    mSwapChainDesc.BufferCount = SwapChainBufferCount;
    mSwapChainDesc.OutputWindow = mhMainWnd;
    mSwapChainDesc.Windowed = true;
    mSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    mSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    //交换链需要通过命令队列进行刷新
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &mSwapChainDesc, &mdxgiSwapChain));
}

void D3DApp::CreateRtvAndDsvDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc;
    RtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    RtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc;
    DsvHeapDesc.NumDescriptors = 1;
    DsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&DsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
}

void D3DApp::FlushCommandQueue()
{
    ++mCurrentFence;

    ThrowIfFailed(mCommandQueue->Signal(md3dFence.Get(), mCurrentFence));

    if (md3dFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(md3dFence->SetEventOnCompletion(mCurrentFence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

D3DApp* D3DApp::GetApp()
{
    return mApp;
}

HINSTANCE D3DApp::APPInst() const
{
    return mhAppInst;
}

HWND D3DApp::MainWnd() const
{
    return mhMainWnd;
}

float D3DApp::AspectRatio() const
{
    return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState() const
{
    return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
    if (m4xMsaaState != value)
    {
        m4xMsaaState = value;
        //修改4XMSAA设置之后要重新创建交换链
        CreateSwapChain();
        OnResize();
    }
}

int D3DApp::Run()
{
    MSG msg = { 0 };
    
    mTimer.Reset();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg,0,0,0,PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            mTimer.Tick();

            if (!mAppPaused)
            {
                CalculateFrameStats();
                Update(mTimer);
                Draw(mTimer);
            }
            else
            {
                Sleep(100);
            }
        }
    }
    return (int)msg.wParam;
}

void D3DApp::OnResize()
{
    assert(md3dDevice);
    assert(mdxgiSwapChain);
    assert(mDirectCmdListAlloc);

    //重置之前确保命令队列中没有未执行的命令
    FlushCommandQueue();
    ThrowIfFailed(mDirectCmdListAlloc->Reset());
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));


    //重置所有相关资源
    for (size_t i = 0;i != SwapChainBufferCount;++i)
    {
        mSwapChainBuffer[i].Reset();
    }
    mDepthStencilBuffer.Reset();

    //重新设置交换链后台缓冲区大小
    ThrowIfFailed(mdxgiSwapChain->ResizeBuffers(
                            SwapChainBufferCount, 
                            mClientWidth, 
                            mClientHeight,
                            mBackBufferFormat,
                            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    mCurrentBackBuffer = 0;

    //重新创建后台缓冲区描述符
    CD3DX12_CPU_DESCRIPTOR_HANDLE RtvDescriptorHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (size_t i = 0;i != SwapChainBufferCount;++i)
    {
        //获取缓冲区资源
        ThrowIfFailed(mdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
        //创建Rtv
        md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, RtvDescriptorHeapHandle);
        RtvDescriptorHeapHandle.Offset(1, mRtvDescriptorSize);
    }

    //创建深度/模板缓冲区及描述符
    D3D12_RESOURCE_DESC DepthStencilBufferDesc;
    DepthStencilBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    DepthStencilBufferDesc.Alignment = 0;
    DepthStencilBufferDesc.Width = mClientWidth;
    DepthStencilBufferDesc.Height = mClientHeight;
    DepthStencilBufferDesc.DepthOrArraySize = 1;
    DepthStencilBufferDesc.MipLevels = 1;
    DepthStencilBufferDesc.Format = mDepthStencilFormat;
    DepthStencilBufferDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    DepthStencilBufferDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    DepthStencilBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    DepthStencilBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.f;
    optClear.DepthStencil.Stencil = 0;
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
        D3D12_HEAP_FLAG_NONE, 
        &DepthStencilBufferDesc, 
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&mDepthStencilBuffer)));

    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr,DepthStencilView());

    //转换资源状态
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
    //将命令提交到命令队列，提交前要关闭命令列表
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    //刷新队列，确保执行
    FlushCommandQueue();

    //重置视口与裁剪矩形大小
    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(mClientWidth);
    mScreenViewport.Height = static_cast<float>(mClientHeight);
    mScreenViewport.MinDepth = 0.f;
    mScreenViewport.MaxDepth = 1.f;

    mScissorRect = { 0,0,mClientWidth,mClientHeight };
}

bool D3DApp::Initialize()
{
    if (!InitialMainWindow())
    {
        return false;
    }

    if (!InitialDirect3D())
    {
        return false;
    }

    //进行一次OnResize以进行相关初始化
    OnResize();

    return true;
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    //应用程序激活或休眠时系统会发送WM_ACTIVATE，wParam参数代表应用程序将变成的状态
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            mAppPaused = true;
            mTimer.Stop();
        }
        else
        {
            mAppPaused = false;
            mTimer.Start();
        }
        return 0;
    //窗口大小调整时会发送WM_SIZE消息
    case WM_SIZE:
        //根据窗口大小调整相应参数
        mClientWidth = LOWORD(lParam);
        mClientHeight = HIWORD(lParam);
        //如果窗口最小化了，则程序应该暂停
        if (md3dDevice)
        {
            if (wParam == SIZE_MINIMIZED)
            {
                mAppPaused = true;
                mMinimized = true;
                mMaximized = false;
            }
            //如果窗口最大化，则程序要恢复运行，且窗口应该重新绘制
            else if (wParam == SIZE_MAXIMIZED)
            {
                mAppPaused = false;
                mMinimized = false;
                mMaximized = true;
                OnResize();
            }
            //如果窗口恢复大小
            else if (wParam = SIZE_RESTORED)
            {
                //是否从最小化恢复
                if (mMinimized)
                {
                    mAppPaused = false;
                    mMinimized = false;
                    OnResize();
                }
                //是否从最大化恢复
                else if (mMaximized)
                {
                    mAppPaused = false;
                    mMaximized = false;
                    OnResize();
                }
                //是否正在调整窗口大小
                else if (mResizing)
                {
                    //如果正在调整窗口大小，则不做任何处理，直到调整栏被释放
                }
                else
                {
                    OnResize();
                }
            }
        }
        return 0;
    //当用户正在调整窗口栏,程序需要暂停，直到用户完成调整再恢复程序，进行缓冲区大小调整等操作
    case WM_ENTERSIZEMOVE:
        mAppPaused = true;
        mResizing = true;
        mTimer.Stop();
        return 0;
    //当用户完成窗口栏大小调整（释放）,程序需要恢复，且重新调整缓冲区大小
    case WM_EXITSIZEMOVE:
        mAppPaused = false;
        mResizing = false;
        mTimer.Start();
        OnResize();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    //当某一菜单处于激活状态，且用户按下的既不是mnemonic key也不是acceleratorkey时，发送WM_MENUCHARA消息
    case WM_MENUCHAR:
        //按下组合键alt+enter时不发出beep蜂鸣声
        return MAKELRESULT(0, MNC_CLOSE);
    //捕获此消息防止窗口过小
    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
        return 0;
    //处理鼠标输入,使用GET_X_LPARAM与GET_Y_LPARAM宏，需要包含<Windowsx.h>
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if ((int)wParam == VK_F2)
        {
            Set4xMsaaState(!m4xMsaaState);
        }
        return 0;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}


void D3DApp::CalculateFrameStats()
{
    static int FrameCount = 0;
    static float TimeElapsed = 0.f;

    ++FrameCount;

    if (mTimer.TotalTime() - TimeElapsed >= 1.f)
    {
        float fps = (float)FrameCount;
        float mspf = 1000.f / fps;

        std::wstring fpsStr = std::to_wstring(fps);
        std::wstring mspfStr = std::to_wstring(mspf);

        std::wstring WindowText = mMainWindowCaption + L"  fps:  " + fpsStr + L"  mspf:  " + mspfStr;

        SetWindowText(mhMainWnd, WindowText.c_str());

        FrameCount = 0;
        TimeElapsed += 1.f;
    }
}

void D3DApp::LogAdapters()
{
    UINT i = 0;

    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;

    //通过IDXGIFactory的EnumAdapters来枚举所有的显示适配器
    while (mdxgiFactory->EnumAdapters(i,&adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());
        adapterList.push_back(adapter);
        ++i;
    }

    for (size_t i = 0;i < adapterList.size();++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;

    //通过IDXGIAdapter的EnumOutputs方法来枚举所有的显示输出
    while (adapter->EnumOutputs(i,&output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, mBackBufferFormat);

        ReleaseCom(output);
        ++i;
    }

}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* Output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    //以nullptr作为参数可获得符合条件的显示模式个数
    Output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    //以数组容器首元素的指针为参数，可获得符合条件的所有的显示模式
    Output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (const auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;

        std::wstring text = L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"RefreshRate = " + std::to_wstring(n) + L"/" + std::to_wstring(d) + L"\n";

        OutputDebugString(text.c_str());
    }
}

