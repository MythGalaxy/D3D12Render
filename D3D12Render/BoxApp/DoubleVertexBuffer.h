#pragma once
#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include "Common/d3dUtil.h"

struct DVBMeshGeometry
{
    std::string Name;

    Microsoft::WRL::ComPtr<ID3DBlob> VertexPosBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> VertexColorBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexPosBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexColorBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexPosBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexColorBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    //顶点缓冲区相关数据
    UINT VertexPosByteStride = 0;
    UINT VertexPosBufferByteSize = 0;
    UINT VertexColorByteStride = 0;
    UINT VertexColorBufferByteSize = 0;
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
    UINT IndexBufferByteSize = 0;

    std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    //获取位置顶点缓冲区描述符
    D3D12_VERTEX_BUFFER_VIEW GetVertexPosBufferView() const
    {
        D3D12_VERTEX_BUFFER_VIEW vpbView;
        vpbView.BufferLocation = VertexPosBufferGPU->GetGPUVirtualAddress();
        vpbView.StrideInBytes = VertexPosByteStride;
        vpbView.SizeInBytes = VertexPosBufferByteSize;

        return vpbView;
    }

    //获取颜色顶点缓冲区描述符
    D3D12_VERTEX_BUFFER_VIEW GetVertexColorBufferView() const 
    {
        D3D12_VERTEX_BUFFER_VIEW vcbView;
        vcbView.BufferLocation = VertexColorBufferGPU->GetGPUVirtualAddress();
        vcbView.StrideInBytes = VertexColorByteStride;
        vcbView.SizeInBytes = VertexColorBufferByteSize;

        return vcbView;
    }

    //获取索引缓冲区描述符
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const 
    {
        D3D12_INDEX_BUFFER_VIEW ibView;
        ibView.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibView.Format = IndexFormat;
        ibView.SizeInBytes = IndexBufferByteSize;

        return ibView;
    }

    //upload堆把数据复制到default堆后可以释放其中资源
    void DisposeUploaders()
    {
        VertexPosBufferUploader = nullptr;
        VertexColorBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
};