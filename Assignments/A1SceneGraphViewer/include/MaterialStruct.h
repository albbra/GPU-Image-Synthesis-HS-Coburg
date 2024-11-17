// MaterialStruct.h
#ifndef MATERIAL_STRUCT
#define MATERIAL_STRUCT

#include <ConstantBufferD3D12.hpp>
#include <gimslib/types.hpp>

/// <summary>
/// Material Information.
/// </summary>
struct Material
{
  ConstantBufferD3D12                          materialConstantBuffer; //! Constant buffer for the material.
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;      //! Descriptor Heap for the textures.
};

#endif // MATERIAL_STRUCT