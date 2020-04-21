#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"


struct ConstantObject
{
    DirectX::XMFLOAT4X4 mWorldViewProj = MathHelper::Identity4x4();
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

class BoxApp : public D3DApp
{
public:
    BoxApp(HINSTANCE hInst);
    BoxApp(const BoxApp& rhs) = delete;
    BoxApp& operator=(const BoxApp& rhs) = delete;
    ~BoxApp();

    virtual bool Initialize() override;
private:
    //相关函数

    //首先是6个需要继承实现的虚函数
    //窗口大小改变时的函数方法
    virtual void OnResize() override;
    //Tick方法
    virtual void Update(const GameTimer& gt) override;
    //绘制方法Draw
    virtual void Draw(const GameTimer& gt) override;
    //鼠标输入消息的响应函数
    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

    //以下是各类初始化函数，用于初始化渲染管线需要用到的各类资源
    //创建描述符堆（用于存储常量缓冲区描述符，顶点缓冲区以及索引缓冲区描述符不需要放在堆中）
    void BuildDescriptorHeap();
    //创建常量缓冲区
    void BuildConstantBuffer();
    //创建根签名(!!!)
    void BuildRootSignature();
    //读取Shader以及初始化输入布局描述
    void BuildShadersAndInputLayout();
    //创建待绘制的几何图形顶点及索引信息
    void BuildMeshGeometry();
    //创建流水线状态对象PSO
    void BuildPSO();

private:
    //相关数据变量以及对象指针变量

    //流水线状态PSO
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;

    //根签名，用于指示各种资源应绑定到哪个着色器上
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

    //需要常量缓冲区，需要创建相应的缓冲区资源以及缓冲区描述符，要存储缓冲区描述符，还需要对应的描述符堆
    //本程序中常量缓冲区主要存储由世界坐标到齐次裁剪空间坐标的转换矩阵，可能需要每帧变动（摄像机移动之后相应的观察矩阵就会变）
    //所以常量缓冲区应该用上传堆UploadHeap，便于CPU修改，在辅助类UploadBuffer的构造函数中我们会直接创建对应的堆资源
    //另外，还需要创建对应的结构体以存储变换矩阵

    //常量缓冲区资源
    std::unique_ptr<UploadBuffer<ConstantObject>> mCBObj = nullptr;
    //常量缓冲区描述符堆
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCBViewHeap = nullptr;

    //还需要相应的顶点缓冲区与索引缓冲区，那么要创建对应的顶点结构体，输入布局描述
    //顶点缓冲区中的数据一般只供GPU读取，要放在默认堆中
    //要在默认堆中写入数据，需要创建上传堆（UploadBuffer），把内存中的顶点数据写入到上传堆中，再将上传堆的数据复制到默认堆
    //我们利用d3dUtil::CreateDefaultBuffer辅助函数来完成这一系列功能
    //另外，有了顶点缓冲区，我们还需要索引缓冲区，每一个特定的3D图形都有其自己的顶点与索引，也就需要特定的缓冲区资源
    //可以把这些指针都放置在一个结构体中，方便调用，即MeshGeometry,结构体内有顶点缓冲区与索引缓冲区数据的内存地址、默认堆(及描述符)、上传堆(及描述符)
    //那么，我们需要一个指向MeshGeometry对象的指针来表示要绘制的图形
    //一个MeshGeometry中可以容纳多个图形的顶点以及索引数据，单个图形的顶点以及索引数据可以利用相应的偏移量来从MeshGeometry中取得
    //那么，就可以利用基址以及相应偏移量来表示一个个特定的图元，这些基址以及偏移量数据组织在SubMeshGeometry结构体中

    //顶点输入布局
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    //待绘制图形数据
    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

    //同样，我们还需要对应的编译好的Shader代码，此代码以ID3DBlob格式存储

    //顶点着色器
    Microsoft::WRL::ComPtr<ID3DBlob> mVSByteCode = nullptr;
    //像素着色器
    Microsoft::WRL::ComPtr<ID3DBlob> mPSByteCode = nullptr;

    //对应的变换矩阵
    DirectX::XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    //假设待绘制物体固定在原点，那么观察摄像机可以认为是在以待绘制物体为中心的球面坐标系中
    //应用程序类中记录坐标系相关参数
    float mRadius = 5.0f;                   //摄像机离物体中心的距离（球面半径）
    float mTheta = DirectX::XM_PIDIV2;      //从原点指向视点的向量在xOy平面上的投影向量与x轴正方向的夹角，弧度制
    float mPhi = DirectX::XM_PIDIV4;        //从原点指向视点的向量与xOy平面的法线量的夹角，弧度制

    //上一帧时鼠标的位置
    POINT mLastMousePos;
    
};

BoxApp::BoxApp(HINSTANCE hInst) :D3DApp(hInst)
{

}

BoxApp::~BoxApp()
{

}

bool BoxApp::Initialize()
{
    //先进行基础初始化
    if (!D3DApp::Initialize())
    {
        return false;
    }

    //重置命令列表，复用相应内存资源
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(),nullptr));

    BuildDescriptorHeap();
    BuildConstantBuffer();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildMeshGeometry();
    BuildPSO();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    //刷新命令队列
    FlushCommandQueue();

    return true;
}

void BoxApp::BuildDescriptorHeap()
{
    //填写描述结构体，然后直接创建即可
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(mCBViewHeap.GetAddressOf())));
}

void BoxApp::BuildConstantBuffer()
{
    //创建常量缓冲区（通过UploadBuffer构造函数完成相关内容）
    mCBObj = std::make_unique<UploadBuffer<ConstantObject>>(md3dDevice.Get(), 1, true);
    //接着需要创建对应的常量缓冲区描述符,先对其进行描述
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    //这个描述结构体需要描述符对应的缓冲区的内存地址，以及缓冲区大小,而常量缓冲区大小本身必须是256b的整数倍
    //先获取对应常量缓冲区的地址
    D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferAddress = mCBObj->Resource()->GetGPUVirtualAddress();
    //若此常量缓冲区包含了许多个常量数据对象，那么需要做相应偏移才能得到真正的地址，这里只存储一个对象，把索引定为0
    UINT cbIndex = 0;
    UINT cbobjSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ConstantObject));
    ConstantBufferAddress += cbIndex * cbobjSize;
    //再获取对应大小
    UINT cbvobjSizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ConstantObject));
    //填写描述体
    cbvDesc.BufferLocation = ConstantBufferAddress;
    cbvDesc.SizeInBytes = cbvobjSizeInBytes;
    //创建对应描述符
    md3dDevice->CreateConstantBufferView(&cbvDesc, mCBViewHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
    //创建根签名，将相应资源绑定到着色器对应的寄存器，这里我们要绑定常量缓冲区

}

void BoxApp::BuildShadersAndInputLayout()
{

}

void BoxApp::BuildMeshGeometry()
{

}

void BoxApp::BuildPSO()
{

}

void BoxApp::OnResize()
{

}

void BoxApp::Update(const GameTimer& gt)
{

}

void BoxApp::Draw(const GameTimer& gt)
{

}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{

}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{

}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{

}


