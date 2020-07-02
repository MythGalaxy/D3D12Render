#pragma once

#include "d3dUtil.h"
#include "UploadBuffer.h"

//物体常量缓冲区，存储物体的世界矩阵,每当物体的世界矩阵变动的时候更新一次
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};

//渲染过程常量缓冲区,存储每次渲染过程需要用到的常量
struct PassConstants
{
    //观察矩阵
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    //投影矩阵
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();

    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();

    //视点/摄像机位置
    DirectX::XMFLOAT3 EyePosW = { 0.0f,0.0f,0.0f };

    float cbPerObjectPad1 = 0.0f;

    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f,0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f,0.0f };

    float NearZ = 0.0f;
    float FarZ = 0.0f;

    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
};

//顶点结构体
struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

struct FrameResource
{
public:

    //构造函数与析构函数
    //不希望帧资源可以被复制，所以把他们的复制构造函数与复制运算符都定义为delete
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    //每个帧资源都应该有自己的命令分配器
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAllocator;

    //帧资源中存储本帧绘制时渲染流水线所需要的常量缓冲区数据
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
};