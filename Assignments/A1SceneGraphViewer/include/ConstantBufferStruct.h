// ConstantBufferStruct.h
#ifndef CONSTANT_BUFFER_STRUCT
#define CONSTANT_BUFFER_STRUCT

#include "LightStruct.h"
#include <gimslib/types.hpp>

struct ConstantBuffer
{
  gims::f32m4 projectionMatrix;
  gims::f32v3 cameraPosition;
  gims::ui32  numOfLights;
  Light       Lights[8];
};
#endif // CONSTANT_BUFFER_STRUCT
