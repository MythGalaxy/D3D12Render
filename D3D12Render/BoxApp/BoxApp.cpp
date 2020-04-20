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

private:
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
    
};

BoxApp::BoxApp(HINSTANCE hInst) :D3DApp(hInst)
{

}

BoxApp::~BoxApp()
{

}
