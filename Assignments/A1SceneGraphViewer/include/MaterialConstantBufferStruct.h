// MaterialConstantBufferStruct.h
#ifndef MATERIAL_CONSTANT_BUFFER_STRUCT
#define MATERIAL_CONSTANT_BUFFER_STRUCT

#include <gimslib/types.hpp>

/// <summary>
/// Material information per mesh that will be uploaded to the GPU.
/// </summary>
struct MaterialConstantBuffer
{
  gims::f32v4 ambientColor             = gims::f32v4(0); //! Ambient Color.
  gims::f32v4 diffuseColor             = gims::f32v4(0); //! Diffuse Color.
  gims::f32v4 emissionColor            = gims::f32v4(0); //! Emission Color.
  gims::f32v4 specularColorAndExponent = gims::f32v4(0); //! xyz: Specular Color, w: Specular Exponent.
};

#endif // MATERIAL_CONSTANT_BUFFER_STRUCT