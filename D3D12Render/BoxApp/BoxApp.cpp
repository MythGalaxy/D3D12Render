﻿#include "../Common/d3dApp.h"
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
    //创建根签名。根签名中有一系列根参数，这些根参数描述了应该被绑定到渲染流水线上的相关资源
    //这里只需要把常量缓冲区绑定到渲染流水线上，那么根签名只需这一个"槽"(即根参数)
    //需要利用D3D12_ROOT_PARAMETER描述结构体来描述这个根参数
    //描述内容包括根参数所代表的资源类型（这里是常量缓冲区），数量（这里是1个）以及基准着色器（这里是0）
    //表示的内容是：绘制过程中，会将[1]个[常量缓冲区]资源绑定到对应的0号寄存器上[b0]
    
    //创建根签名需要先将根签名描述布局进行序列化处理，也需要先创建根签名的描述结构体
    //根签名的描述结构体需要填写对应根参数数组，所以先配置对应的根参数数组

    //利用辅助结构体CD3DX12_ROOT_PARAMETER,只有一个根参数
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];
    //此根参数的形式是描述符表DescriptorTable
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    //设置根参数格式
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    //描述根签名,利用辅助结构体CD3DX12_ROOT_SIGNATURE_DESC
    CD3DX12_ROOT_SIGNATURE_DESC rootSignature(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    //根签名描述完毕，但是要真正创建，需要将其序列化，序列化的数据以ID3DBlob来表示
    Microsoft::WRL::ComPtr<ID3DBlob> SerializedRootSig = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> ErrBlob = nullptr;
    //序列化
    HRESULT hr = D3D12SerializeRootSignature(&rootSignature, D3D_ROOT_SIGNATURE_VERSION_1, SerializedRootSig.GetAddressOf(), ErrBlob.GetAddressOf());
    //当出现异常，输出异常信息
    if (ErrBlob != nullptr)
    {
        OutputDebugStringA((char*)ErrBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    //创建根签名
    ThrowIfFailed(md3dDevice->CreateRootSignature(0, SerializedRootSig->GetBufferPointer(), SerializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShadersAndInputLayout()
{
    //这里我们直接读取离线编译好的VS与PS cso文件
    mVSByteCode = d3dUtil::LoadBinary(L"D3D12Render\\Shader\\BoxApp\\VS.cso");
    mPSByteCode = d3dUtil::LoadBinary(L"D3D12Render\\Shader\\BoxApp\\PS.cso");

    //输入布局描述
    mInputLayout = 
    {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };
}

void BoxApp::BuildMeshGeometry()
{
    //创建待绘制的物体
    //每一个待绘制的物体的顶点与索引信息，需要建立在内存中，然后从内存复制到UploadHeap，再从UploadHeap复制到GPU可读取的DefaultHeap
    //用MeshGeometry结构体来合并存储所有的待绘制物体的顶点与索引信息（虽然此例中暂时只绘制一个长方体）
    //用SubmeshGeometry存储具体待绘制物体的顶点与索引的偏移信息，然后从MeshGeometry中取得真正的顶点与索引

    //先直接建立顶点信息,以中心点为原点,向右的单位向量为(1,0,0),向前的单位向量为(0,1,0),向上的单位向量为(0,0,1)
    //这是一个边长为2.0f的立方体

    /*
              *0------*1
             /       /|   
            *2------*3| 
            | *4    | *5
            |       |/
            *6------*7
    */

    std::array<Vertex,8> vertices = 
    {
        Vertex({DirectX::XMFLOAT3(-1.0f,-1.0f,1.0f),DirectX::XMFLOAT4(DirectX::Colors::Black)}),
        Vertex({DirectX::XMFLOAT3(1.0f,-1.0f,1.0f),DirectX::XMFLOAT4(DirectX::Colors::White)}),
        Vertex({DirectX::XMFLOAT3(-1.0f,1.0f,1.0f),DirectX::XMFLOAT4(DirectX::Colors::Red)}),
        Vertex({DirectX::XMFLOAT3(1.0f,1.0f,1.0f),DirectX::XMFLOAT4(DirectX::Colors::Green)}),
        Vertex({DirectX::XMFLOAT3(-1.0f,-1.0f,-1.0f),DirectX::XMFLOAT4(DirectX::Colors::Blue)}),
        Vertex({DirectX::XMFLOAT3(1.0f,-1.0f,-1.0f),DirectX::XMFLOAT4(DirectX::Colors::Yellow)}),
        Vertex({DirectX::XMFLOAT3(-1.0f,1.0f,-1.0f),DirectX::XMFLOAT4(DirectX::Colors::Cyan)}),
        Vertex({DirectX::XMFLOAT3(1.0f,1.0f,-1.0f),DirectX::XMFLOAT4(DirectX::Colors::Magenta)})
    };

    //索引信息,立方体6个面，每个面由2个三角形组成，每个三角形有3个顶点，用3个索引值(顶点索引值见上面的注释)来表示
    std::array<std::uint16_t,36> indices = 
    {
        //顶面
        0,1,2,
        1,3,2,
        //底面
        4,6,5,
        5,6,7,
        //前面
        2,3,6,
        3,7,6,
        //后面
        0,4,1,
        1,4,5,
        //左面
        0,4,2,
        2,4,6,
        //右面
        1,5,3,
        3,5,7
    };

    //计算资源大小
    const UINT vbByteSize = vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

    //创建资源存储对象
    mBoxGeo = std::make_unique<MeshGeometry>();
    mBoxGeo->Name = "BoxGeo";

    //创建内存块
    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
    //将数据复制到对应内存块
    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    //创建对应的GPU资源，用到了d3dUtil::CreateDefaultBuffer方法
    mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(), mCommandList.Get(), mBoxGeo->VertexBufferCPU->GetBufferPointer(), vbByteSize, mBoxGeo->VertexBufferUploader);
    mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(), mCommandList.Get(), mBoxGeo->IndexBufferCPU->GetBufferPointer(), ibByteSize, mBoxGeo->IndexBufferUploader);

    mBoxGeo->VertexByteStride = sizeof(Vertex);
    mBoxGeo->VertexBufferByteSize = vbByteSize;
    mBoxGeo->IndexBufferByteSize = ibByteSize;
    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;

    //次表面索引
    SubmeshGeometry submesh;
    submesh.IndexCount = indices.size();
    submesh.BaseVertexLocation = 0;
    submesh.StartIndexLocation = 0;

    mBoxGeo->DrawArgs["Box"] = submesh;
}

void BoxApp::BuildPSO()
{
    //创建流水线状态对象，先填写对应描述结构体
    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
    ZeroMemory(&PSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    //根签名
    PSODesc.pRootSignature = mRootSignature.Get();
    //输入描述布局
    PSODesc.InputLayout = { mInputLayout.data(),(UINT)mInputLayout.size() };
    //顶点着色器
    PSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mVSByteCode->GetBufferPointer()),
        mVSByteCode->GetBufferSize()
    };
    //像素着色器
    PSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mPSByteCode->GetBufferPointer()),
        mPSByteCode->GetBufferSize()
    };
    //光栅器状态,设置为默认值
    PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    PSODesc.SampleMask = UINT_MAX;
    PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    //渲染目标数量
    PSODesc.NumRenderTargets = 1;
    //渲染目标格式
    PSODesc.RTVFormats[0] = mBackBufferFormat;
    //采样相关
    PSODesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    PSODesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    //深度/模板缓冲区格式
    PSODesc.DSVFormat = mDepthStencilFormat;

    //创建PSO
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&mPSO)));
}

void BoxApp::OnResize()
{
    D3DApp::OnResize();

    //窗口大小改变，意味着纵横比可能改变，需要重新修改齐次裁剪矩阵(投影变换矩阵)
    DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    //更新mProj
    DirectX::XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
    //由于窗口大小以及摄像机位置可能改变，需要每次Tick都更新常量缓冲区的gWorldViewProj矩阵
    //将摄像机的坐标表示为笛卡尔坐标
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float y = mRadius * sinf(mPhi) * sinf(mTheta);
    float z = mRadius * cosf(mPhi);
    //准备建立观察矩阵
    DirectX::XMVECTOR Pos = DirectX::XMVectorSet(x, y, z, 1.0f);
    DirectX::XMVECTOR Target = DirectX::XMVectorZero();
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(Pos, Target, up);
    DirectX::XMStoreFloat4x4(&mView, V);

    //准备更新WorldViewProj
    DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&mWorld);
    DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&mProj);
    DirectX::XMMATRIX WorldViewProj = W * V * P;

    //更新到常量缓冲区
    ConstantObject constObj;
    DirectX::XMStoreFloat4x4(&constObj.mWorldViewProj, WorldViewProj);
    mCBObj->CopyData(0, constObj);
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


