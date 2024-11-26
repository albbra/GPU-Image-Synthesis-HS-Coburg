// ConstantBufferStruct.h
#ifndef CONSTANT_BUFFER_STRUCT
#define CONSTANT_BUFFER_STRUCT

#include <gimslib/types.hpp>

struct ConstantBuffer
{
  gims::f32m4 projectionMatrix;
};

#endif // CONSTANT_BUFFER_STRUCT