// TriangleMeshD3D12.cpp

#include "TriangleMeshD3D12.hpp"
#include "VertexStruct.h"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>

const std::vector<D3D12_INPUT_ELEMENT_DESC> TriangleMeshD3D12::m_inputElementDescs = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

TriangleMeshD3D12::TriangleMeshD3D12(gims::f32v3 const* const positions, gims::f32v3 const* const normals,
                                     gims::f32v3 const* const textureCoordinates, gims::ui32 nVertices,
                                     gims::ui32v3 const* const indexBuffer, gims::ui32 nIndices,
                                     gims::ui32 materialIndex, const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                                     const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
    : m_nIndices(nIndices)
    , m_vertexBufferSize(static_cast<gims::ui32>(nVertices * sizeof(Vertex)))
    , m_indexBufferSize(static_cast<gims::ui32>(nIndices * sizeof(gims::ui32)))
    , m_aabb(positions, nVertices)
    , m_materialIndex(materialIndex)
{
  // Assignment 2
  (void)positions;
  (void)normals;
  (void)textureCoordinates;
  (void)indexBuffer;
  (void)device;
  (void)commandQueue;
}

void TriangleMeshD3D12::addToCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
  (void)commandList;
  // Assignment 2
}

const AABB TriangleMeshD3D12::getAABB() const
{
  return m_aabb;
}

const gims::ui32 TriangleMeshD3D12::getMaterialIndex() const
{
  return m_materialIndex;
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& TriangleMeshD3D12::getInputElementDescriptors()
{
  return m_inputElementDescs;
}

TriangleMeshD3D12::TriangleMeshD3D12()
    : m_nIndices(0)
    , m_vertexBufferSize(0)
    , m_indexBufferSize(0)
    , m_materialIndex((gims::ui32)-1)
{
}
