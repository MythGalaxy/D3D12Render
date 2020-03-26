
#include "d3dApp.h"

D3DApp* D3DApp::mApp = nullptr;
D3DApp::D3DApp(HINSTANCE hInstance) :mhAppInst(hInstance)
{
    assert(mApp == nullptr);
    mApp = this;
}

D3DApp::D3DApp()
{
    assert(mApp == nullptr);
    mApp = this;
}

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
    D3D12_COMMAND_QUEUE_DESC mCommandQueueDesc;
    mCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    mCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&mCommandQueueDesc, IID_PPV_ARGS(&mCommandQueue)));
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mDirectCmdListAlloc)));
    ThrowIfFailed(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
    mCommandList->Close();
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

}

bool D3DApp::Initialize()
{

}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

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
    while (adapter->EnumOutputs(i,&output))
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

