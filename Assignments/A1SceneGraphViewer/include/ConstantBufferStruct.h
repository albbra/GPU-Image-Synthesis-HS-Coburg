// ConstantBufferStruct.h
#ifndef CONSTANT_BUFFER_STRUCT
#define CONSTANT_BUFFER_STRUCT

#include <gimslib/types.hpp>

struct ConstantBuffer
{
  gims::f32m4 projectionMatrix;
  gims::f32v3 cameraPosition;
  gims::f32 pad;
  gims::f32v3 lightPosition;
};

#endif // CONSTANT_BUFFER_STRUCT