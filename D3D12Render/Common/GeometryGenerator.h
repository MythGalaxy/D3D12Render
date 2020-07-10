//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
//***************************************************************************************

#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>


class GeometryGenrator
{
public:

    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

    //定义顶点结构体，内含Position位置、法向量Normal、切向量TangentU、UV纹理坐标四个属性
    struct Vertex
    {
        //构造函数
        Vertex() {}
        Vertex(const DirectX::XMFLOAT3& p, const DirectX::XMFLOAT3& n, const DirectX::XMFLOAT3 t, const DirectX::XMFLOAT2 uv): 
        Position(p),Normal(n),TangentU(t),TexC(uv){}
        Vertex(
            float px, float py, float pz,
            float nx, float ny, float nz,
            float tx, float ty, float tz,
            float u, float v) :
            Position(px, py, pz), Normal(nx, ny, nz), TangentU(tx, ty, tz), TexC(u, v) {}

        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 TangentU;
        DirectX::XMFLOAT2 TexC;

    };

    //欲构造的几何体Mesh所需的数据结构体
    struct MeshData
    {
        //顶点与索引
        std::vector<Vertex> Vertices;
        std::vector<uint32> Indices32;

        std::vector<uint16>& GetIndices16()
        {
            if (mIndices16.empty())
            {
                mIndices16.resize(Indices32.size());
                for (size_t i = 0;i != Indices32.size();++i)
                {
                    mIndices16[i] = static_cast<uint16>(Indices32[i]);
                }
            }
            return mIndices16;
        }

    private:
        std::vector<uint16> mIndices16;
    };

    //生成长方体Box,中心位于原点
    MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);

    //生成柱体,中心位于原点,stackCount为层数,sliceCount为每一层的顶点数，sliceCount越大，横截面越接近圆形
    MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount,uint32 stackCount);

    //生成球体，中心位于原点，算法与生成柱体算法类似
    MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

    //生成几何球体，中心位于原点，通过对一个正二十面体进行指定次数的曲面细分来完成
    MeshData CreateGeoSphere(float radius, uint32 numSubDivisions);

    //生成一个由小方格组成的平面，中心在原点,有m行n列
    MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

    //生成quad
    MeshData CreateQuad(float x, float y, float w, float h, float depth);

private:
    //曲面细分函数
    void Subdivide(MeshData& meshData);

    //求中点函数，曲面细分函数中调用此函数来获取细分之后新增的顶点的相应数据
    Vertex MidPoint(const Vertex& v0, const Vertex& v1);

    //柱体的顶面与底面要特殊创建，需要两个函数
    void BuildCylinderTopCap(float bottomRadius,float topRadius,float height, uint32 sliceCount, uint32 stackCount,MeshData& meshData);
    void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
};


//class GeometryGenerator
//{
//public:
//
//    using uint16 = std::uint16_t;
//    using uint32 = std::uint32_t;
//
//	struct Vertex
//	{
//		Vertex(){}
//        Vertex(
//            const DirectX::XMFLOAT3& p, 
//            const DirectX::XMFLOAT3& n, 
//            const DirectX::XMFLOAT3& t, 
//            const DirectX::XMFLOAT2& uv) :
//            Position(p), 
//            Normal(n), 
//            TangentU(t), 
//            TexC(uv){}
//		Vertex(
//			float px, float py, float pz, 
//			float nx, float ny, float nz,
//			float tx, float ty, float tz,
//			float u, float v) : 
//            Position(px,py,pz), 
//            Normal(nx,ny,nz),
//			TangentU(tx, ty, tz), 
//            TexC(u,v){}
//
//        DirectX::XMFLOAT3 Position;
//        DirectX::XMFLOAT3 Normal;
//        DirectX::XMFLOAT3 TangentU;
//        DirectX::XMFLOAT2 TexC;
//	};
//
//	struct MeshData
//	{
//		std::vector<Vertex> Vertices;
//        std::vector<uint32> Indices32;
//
//        std::vector<uint16>& GetIndices16()
//        {
//			if(mIndices16.empty())
//			{
//				mIndices16.resize(Indices32.size());
//				for(size_t i = 0; i < Indices32.size(); ++i)
//					mIndices16[i] = static_cast<uint16>(Indices32[i]);
//			}
//
//			return mIndices16;
//        }
//
//	private:
//		std::vector<uint16> mIndices16;
//	};
//
//	///<summary>
//	/// Creates a box centered at the origin with the given dimensions, where each
//    /// face has m rows and n columns of vertices.
//	///</summary>
//    MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);
//
//	///<summary>
//	/// Creates a sphere centered at the origin with the given radius.  The
//	/// slices and stacks parameters control the degree of tessellation.
//	///</summary>
//    MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);
//
//	///<summary>
//	/// Creates a geosphere centered at the origin with the given radius.  The
//	/// depth controls the level of tessellation.
//	///</summary>
//    MeshData CreateGeosphere(float radius, uint32 numSubdivisions);
//
//	///<summary>
//	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
//	/// The bottom and top radius can vary to form various cone shapes rather than true
//	// cylinders.  The slices and stacks parameters control the degree of tessellation.
//	///</summary>
//    MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);
//
//	///<summary>
//	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
//	/// at the origin with the specified width and depth.
//	///</summary>
//    MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);
//
//	///<summary>
//	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
//	///</summary>
//    MeshData CreateQuad(float x, float y, float w, float h, float depth);
//
//private:
//	void Subdivide(MeshData& meshData);
//    Vertex MidPoint(const Vertex& v0, const Vertex& v1);
//    void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
//    void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
//};

