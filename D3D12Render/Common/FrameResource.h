#pragma once

#include "d3dUtil.h"

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

};