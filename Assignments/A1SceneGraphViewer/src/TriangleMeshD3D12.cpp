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
    , m_vertexBuffer()
    , m_vertexBufferView()
    , m_indexBuffer()
    , m_indexBufferView()
{
  if (!positions || !normals || !textureCoordinates || !indexBuffer || !device || !commandQueue)
  {
    throw std::invalid_argument("Invalid arguments passed to TriangleMeshD3D12 constructor.");
  }

  // Create vertex data
  std::vector<Vertex> vertexBufferCPU(nVertices);
  for (gims::ui32 i = 0; i < nVertices; ++i)
  {
    vertexBufferCPU[i].position = positions[i];
    vertexBufferCPU[i].normal   = normals[i];
    vertexBufferCPU[i].texCoord = {textureCoordinates[i].x, textureCoordinates[i].y};
  }

  // Convert index buffer to a CPU-side array
  std::vector<gims::ui32> indexBufferCPU(nIndices );
  memcpy(indexBufferCPU.data(), indexBuffer, m_indexBufferSize);

  // Instantiate UploadHelper
  gims::UploadHelper uploadHelper(device, std::max(m_vertexBufferSize, m_indexBufferSize));

  // Vertex Buffer Creation and Upload

  const CD3DX12_RESOURCE_DESC   vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);
  const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

  if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
                                             D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer))))
  {
    throw std::runtime_error("Failed to create vertex buffer resource.");
  }

  // Configure vertex buffer view
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = m_vertexBufferSize;
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);

  uploadHelper.uploadBuffer(vertexBufferCPU.data(), m_vertexBuffer, m_vertexBufferSize, commandQueue);

  // Index Buffer Creation and Upload
  const CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferSize);

  if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
                                             D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer))))
  {
    throw std::runtime_error("Failed to create index buffer resource.");
  }

  // Configure index buffer view
  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = m_indexBufferSize;
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  uploadHelper.uploadBuffer(indexBufferCPU.data(), m_indexBuffer, m_indexBufferSize, commandQueue);
}

void TriangleMeshD3D12::addToCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
  if (!commandList)
  {
    throw std::invalid_argument("Command list is null.");
  }

  // Set buffers and topology
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Issue draw command
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
    , m_vertexBufferView()
    , m_indexBufferSize(0)
    , m_materialIndex((gims::ui32)-1)
    , m_indexBufferView()
{
}
