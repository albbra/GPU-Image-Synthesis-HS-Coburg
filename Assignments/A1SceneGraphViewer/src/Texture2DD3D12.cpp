// Texture2DD3D12.cpp

#include "Texture2DD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>

Microsoft::WRL::ComPtr<ID3D12Resource> static createTexture(
    void const* const data, gims::ui32 textureWidth, gims::ui32 textureHeight,
    const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
{
  Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

  D3D12_RESOURCE_DESC textureDescription = {};
  textureDescription.MipLevels           = 1;
  textureDescription.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDescription.Width               = textureWidth;
  textureDescription.Height              = textureHeight;
  textureDescription.Flags               = D3D12_RESOURCE_FLAG_NONE;
  textureDescription.DepthOrArraySize    = 1;
  textureDescription.SampleDesc.Count    = 1;
  textureDescription.SampleDesc.Quality  = 0;
  textureDescription.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  if (FAILED(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDescription,
                                                D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&textureResource))))
  {
    throw std::runtime_error("Failed to create texture resource.");
  }

  gims::UploadHelper uploadHelper(device, GetRequiredIntermediateSize(textureResource.Get(), 0, 1));
  uploadHelper.uploadTexture(data, textureResource, textureWidth, textureHeight, commandQueue);

  return textureResource;
}

Texture2DD3D12::Texture2DD3D12(std::filesystem::path path, const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                               const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
{
  const std::string fileName      = path.generic_string();
  const char* const fileNameCStr  = fileName.c_str();
  gims::i32         textureWidth  = {0};
  gims::i32         textureHeight = {0};
  gims::i32         textureComp   = {0};

  std::unique_ptr<gims::ui8, void (*)(void*)> image(
      stbi_load(fileNameCStr, &textureWidth, &textureHeight, &textureComp, 4), &stbi_image_free);
  if (image.get() == nullptr)
  {
    throw std::exception("Error loading texture.");
  }

  m_textureResource = createTexture(image.get(), textureWidth, textureHeight, device, commandQueue);
}
Texture2DD3D12::Texture2DD3D12(gims::ui8v4 const* const data, gims::ui32 width, gims::ui32 height,
                               const Microsoft::WRL::ComPtr<ID3D12Device>&       device,
                               const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)

{
  m_textureResource = createTexture(data, width, height, device, commandQueue);
}

void Texture2DD3D12::addToDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>&         device,
                                         const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
                                         gims::i32                                           descriptorIndex) const
{
  // Describe the SRV
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM; // Assuming RGBA8 format
  srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip       = 0;
  srvDesc.Texture2D.MipLevels             = 1;
  srvDesc.Texture2D.ResourceMinLODClamp   = 0.0f;

  // Get the descriptor handle
  CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
  srvHandle.Offset(descriptorIndex, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

  // Create the SRV
  device->CreateShaderResourceView(m_textureResource.Get(), &srvDesc, srvHandle);
}
