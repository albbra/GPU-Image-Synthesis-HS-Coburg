// ConstantBufferD3D12.cpp

#include "ConstantBufferD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>

ConstantBufferD3D12::ConstantBufferD3D12()
    : m_sizeInBytes(0)
{
}
ConstantBufferD3D12::ConstantBufferD3D12(size_t sizeInBytes, const Microsoft::WRL::ComPtr<ID3D12Device>& device)
    : m_sizeInBytes(sizeInBytes)
{
  (void)device;
  // Assignment 2
}
const Microsoft::WRL::ComPtr<ID3D12Resource>& ConstantBufferD3D12::getResource() const
{
  return m_constantBuffer;
}
void const ConstantBufferD3D12::upload(void const* const data)
{
  if (m_sizeInBytes == 0)
  {
    return;
  }
  (void)data;
  // Assignemt 2
}
