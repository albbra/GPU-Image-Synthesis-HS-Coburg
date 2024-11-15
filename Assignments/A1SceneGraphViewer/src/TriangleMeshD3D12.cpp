#include "TriangleMeshD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
namespace
{
struct Vertex
{
  gims::f32v3 position;
  gims::f32v3 normal;
  gims::f32v2 textureCoordinate;
};
} // namespace

namespace gims
{
const std::vector<D3D12_INPUT_ELEMENT_DESC> TriangleMeshD3D12::m_inputElementDescs = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

TriangleMeshD3D12::TriangleMeshD3D12(f32v3 const* const positions, f32v3 const* const normals,
                                     f32v3 const* const textureCoordinates, ui32 nVertices,
                                     ui32v3 const* const indexBuffer, ui32 nIndices, ui32 materialIndex,
                                     const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue)
    : m_nIndices(nIndices)
    , m_vertexBufferSize(static_cast<ui32>(nVertices * sizeof(Vertex)))
    , m_indexBufferSize(static_cast<ui32>(nIndices * sizeof(ui32)))
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

void TriangleMeshD3D12::addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
  (void)commandList;
// Assignment 2
}

const AABB TriangleMeshD3D12::getAABB() const
{
  return m_aabb;
}

const ui32 TriangleMeshD3D12::getMaterialIndex() const
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
    , m_materialIndex((ui32)-1)
{
}

} // namespace gims
