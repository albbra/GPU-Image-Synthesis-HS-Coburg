// Texture2DD3D12.cpp

#include "Texture2DD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>

Microsoft::WRL::ComPtr<ID3D12Resource> static createTexture(
    void const* const data, 
    gims::ui32 textureWidth,
    gims::ui32 textureHeight,
    const Microsoft::WRL::ComPtr<ID3D12Device>& device,
    const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;

    // Create texture resource description
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Assuming RGBA8 format
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Create the texture resource in the default heap
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    if (FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&textureResource))))
    {
        throw std::runtime_error("Failed to create texture resource.");
    }

    // Create an upload resource
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureResource.Get(), 0, 1);
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    if (FAILED(device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadResource))))
    {
        throw std::runtime_error("Failed to create upload resource.");
    }

    // Map the upload resource and copy texture data into it
    void* mappedData = nullptr;
    CD3DX12_RANGE readRange(0, 0); // No reading
    if (FAILED(uploadResource->Map(0, &readRange, &mappedData)))
    {
        throw std::runtime_error("Failed to map upload resource.");
    }
    memcpy(mappedData, data, static_cast<size_t>(textureWidth * textureHeight * 4)); // RGBA8 = 4 bytes per pixel
    uploadResource->Unmap(0, nullptr);

    // Copy data from the upload resource to the texture resource
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))) ||
        FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
                                         IID_PPV_ARGS(&commandList))))
    {
        throw std::runtime_error("Failed to create command list or allocator.");
    }

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = data;
    textureData.RowPitch = textureWidth * 4;
    textureData.SlicePitch = textureData.RowPitch * textureHeight;

    UpdateSubresources(commandList.Get(), textureResource.Get(), uploadResource.Get(), 0, 0, 1, &textureData);

    // Transition the texture resource to a shader resource state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();

    // Execute the command list
    ID3D12CommandList* ppCommandLists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(1, ppCommandLists);

    // Wait for the GPU to finish
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!fenceEvent ||
        FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
    {
        throw std::runtime_error("Failed to create fence or event.");
    }
    const UINT64 fenceValue = 1;
    commandQueue->Signal(fence.Get(), fenceValue);
    if (fence->GetCompletedValue() < fenceValue)
    {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    CloseHandle(fenceEvent);

    return textureResource;
}

Texture2DD3D12::Texture2DD3D12(std::filesystem::path path, const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                               const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
{
  const std::string fileName      = path.generic_string();
  const char* const fileNameCStr  = fileName.c_str();
  gims::i32  textureWidth  = {0};
  gims::i32  textureHeight = {0};
  gims::i32  textureComp   = {0};

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

void Texture2DD3D12::addToDescriptorHeap(
    const Microsoft::WRL::ComPtr<ID3D12Device>& device,
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
    gims::i32 descriptorIndex) const
{
    // Describe the SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Assuming RGBA8 format
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    // Get the descriptor handle
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    srvHandle.Offset(descriptorIndex, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    // Create the SRV
    device->CreateShaderResourceView(m_textureResource.Get(), &srvDesc, srvHandle);
}
