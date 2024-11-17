// Texture2DD3D12.hpp
#ifndef TEXTURE_2DD3D12_CLASS
#define TEXTURE_2DD3D12_CLASS

#include <d3d12.h>
#include <filesystem>
#include <gimslib/types.hpp>
#include <wrl.h>

/// <summary>
/// A class that represents 2D textures. It supports the format RGBA8_UNORM only.
/// </summary>
class Texture2DD3D12
{
public:
  /// <summary>
  /// Loads a texture from a file and uploads it onto the GPU. Throws an std::exception in cases something goes wrong.
  /// </summary>
  /// <param name="pathToFileName">Path to filename</param>
  /// <param name="device">Device on which the GPU buffers should be created.</param>
  /// <param name="commandQueue">Command queue used to copy the data from the GPU to the GPU.</param>
  Texture2DD3D12(std::filesystem::path pathToFileName, const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                 const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue);

  /// <summary>
  /// Creates a texture from a pointer in memory.
  /// </summary>
  /// <param name="data">Array to a 2D texture. We assume that the data is RGBA8.</param>
  /// <param name="width">Width in texels.</param>
  /// <param name="height">Width in texels.</param>
  /// <param name="device">Device on which the GPU buffers should be created.</param>
  /// /// <param name="commandQueue">Command queue used to copy the data from the GPU to the GPU.</param>
  Texture2DD3D12(gims::ui8v4 const* const data, gims::ui32 width, gims::ui32 height,
                 const Microsoft::WRL::ComPtr<ID3D12Device>&       device,
                 const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue);

  /// <summary>
  /// Adds the texture to a descriptor heap.
  /// </summary>
  /// <param name="device"></param>
  /// <param name="descriptorHeap">The descriptor heap, to which this texture should be added.</param>
  /// <param name="descriptorIndex">The index within the descriptor heap.</param>
  void addToDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>&         device,
                           const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
                           gims::i32                                           offsetInDescriptors) const;

  Texture2DD3D12()                                           = default;
  Texture2DD3D12(const Texture2DD3D12& other)                = default;
  Texture2DD3D12(Texture2DD3D12&& other) noexcept            = default;
  Texture2DD3D12& operator=(const Texture2DD3D12& other)     = default;
  Texture2DD3D12& operator=(Texture2DD3D12&& other) noexcept = default;

private:
  /// <summary>
  /// The texture resource.
  /// </summary>
  Microsoft::WRL::ComPtr<ID3D12Resource> m_textureResource;
};

#endif // TEXTURE_2DD3D12_CLASS
