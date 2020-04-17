#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"


struct ConstantObject
{
    DirectX::XMFLOAT4X4 mWorldViewProj = MathHelper::Identity4x4();
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

    //缓冲区资源
    std::unique_ptr<UploadBuffer<ConstantObject>> mCBObj = nullptr;
    //常量缓冲区描述符堆
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCBViewHeap = nullptr;
};

BoxApp::BoxApp(HINSTANCE hInst):D3DApp(hInst)
{

}

BoxApp::~BoxApp()
{

}
