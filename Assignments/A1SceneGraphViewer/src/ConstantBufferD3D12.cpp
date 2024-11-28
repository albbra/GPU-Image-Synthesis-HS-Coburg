// ConstantBufferD3D12.cpp

#include "ConstantBufferD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <stdexcept>

ConstantBufferD3D12::ConstantBufferD3D12()
    : m_sizeInBytes(0)
{
}

ConstantBufferD3D12::ConstantBufferD3D12(size_t sizeInBytes, const Microsoft::WRL::ComPtr<ID3D12Device>& device)
    : m_sizeInBytes(sizeInBytes)
{
  if (!device || sizeInBytes == 0)
  {
    throw std::invalid_argument("Invalid device or size for constant buffer.");
  }

  // Ensure size is aligned to 256 bytes (minimum alignment required by DirectX 12 constant buffers).
  m_sizeInBytes = (sizeInBytes + 255) & ~255;

  // Describe and create the constant buffer resource.
  D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  D3D12_RESOURCE_DESC   resourceDesc   = CD3DX12_RESOURCE_DESC::Buffer(m_sizeInBytes);

  if (FAILED(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                             D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                             IID_PPV_ARGS(&m_constantBuffer))))
  {
    throw std::runtime_error("Failed to create constant buffer resource.");
  }
}

const Microsoft::WRL::ComPtr<ID3D12Resource>& ConstantBufferD3D12::getResource() const
{
  return m_constantBuffer;
}

void const ConstantBufferD3D12::upload(void const* const data)
{
  if (!data || m_sizeInBytes == 0 || !m_constantBuffer)
  {
    throw std::invalid_argument("Invalid data or uninitialized constant buffer.");
  }

  // Map the constant buffer to CPU memory.
  void* mappedMemory = nullptr;
  if (FAILED(m_constantBuffer->Map(0, nullptr, &mappedMemory)))
  {
    throw std::runtime_error("Failed to map constant buffer.");
  }
  memcpy(mappedMemory, data, m_sizeInBytes);
  m_constantBuffer->Unmap(0, nullptr);
}
