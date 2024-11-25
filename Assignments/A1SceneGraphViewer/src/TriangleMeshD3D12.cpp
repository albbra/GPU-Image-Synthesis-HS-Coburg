// TriangleMeshD3D12.cpp

#include "TriangleMeshD3D12.hpp"
#include "VertexStruct.h"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <stdexcept>

const std::vector<D3D12_INPUT_ELEMENT_DESC> TriangleMeshD3D12::m_inputElementDescs = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

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
  if (!positions || !normals || !textureCoordinates || !indexBuffer || !device || !commandQueue)
  {
    throw std::invalid_argument("Invalid arguments passed to TriangleMeshD3D12 constructor.");
  }

  // Create combined vertex array
  std::vector<Vertex> vertices(nVertices);
  for (gims::ui32 i = 0; i < nVertices; ++i)
  {
    vertices[i].position = positions[i];
    vertices[i].normal   = normals[i];
    vertices[i].texCoord = {textureCoordinates[i].x, textureCoordinates[i].y};
  }

  // Instantiate the UploadHelper
  gims::UploadHelper uploadHelper(device, std::max(m_vertexBufferSize, m_indexBufferSize));

  // Upload vertex buffer
  uploadHelper.uploadBuffer(vertices.data(), m_vertexBuffer, m_vertexBufferSize, commandQueue);

  // Upload index buffer
  uploadHelper.uploadBuffer(indexBuffer, m_indexBuffer, m_indexBufferSize, commandQueue);
}

void TriangleMeshD3D12::addToCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
  if (!commandList)
  {
    throw std::invalid_argument("Command list is null.");
  }

  // Vertex buffer view
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
  vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  vertexBufferView.SizeInBytes    = m_vertexBufferSize;
  vertexBufferView.StrideInBytes  = sizeof(Vertex);

  // Index buffer view
  D3D12_INDEX_BUFFER_VIEW indexBufferView {};
  indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  indexBufferView.SizeInBytes    = m_indexBufferSize;
  indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  // Set buffers on the command list
  commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
  commandList->IASetIndexBuffer(&indexBufferView);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Draw indexed primitives
  commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
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
