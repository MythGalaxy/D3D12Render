#include "Common/d3dApp.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitDirect3DApp : public D3DApp
{
public:
    InitDirect3DApp(HINSTANCE hInstance) :D3DApp(hInstance) {}
    ~InitDirect3DApp() {}

    virtual bool Initialize() override;

private:
    virtual void OnResize() override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(const GameTimer& gt) override;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    //为Dubug版本开启运行时内存检测，方便监督内存泄漏的情况
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        InitDirect3DApp theApp(hInstance);
        if (!theApp.Initialize())
        {
            return 0;
        }
        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

bool InitDirect3DApp::Initialize()
{
    if (!D3DApp::Initialize())
    {
        return false;
    }
    return true;
}

void InitDirect3DApp::OnResize()
{
    D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer & gt)
{

}

void InitDirect3DApp::Draw(const GameTimer & gt)
{
    //重复使用记录命令相关的内存
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(),nullptr));

    
    //对资源状态进行转换，将资源从呈现状态转为渲染目标状态
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(), 
            D3D12_RESOURCE_STATE_PRESENT, 
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    //设置视口和裁剪矩形
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    //!!! 在每帧重新绘制之前，要将后台缓冲区与深度缓冲区的数据清除
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

    //指定将要渲染的缓冲区
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    //再次对资源状态进行转换，从渲染目标状态转为呈现状态
    mCommandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(), 
            D3D12_RESOURCE_STATE_RENDER_TARGET, 
            D3D12_RESOURCE_STATE_PRESENT));

    //完成命令的记录
    ThrowIfFailed(mCommandList->Close());

    //将命令提交到命令队列
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists),cmdsLists);

    //FlushCommandQueue();

    //交换后台缓冲区与前台缓冲区
    ThrowIfFailed(mdxgiSwapChain->Present(0, 0));
    mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SwapChainBufferCount;

    //刷新命令队列
    FlushCommandQueue();
}
