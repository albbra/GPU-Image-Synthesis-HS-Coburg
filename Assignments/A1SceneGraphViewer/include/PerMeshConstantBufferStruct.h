// PerMeshConstantBufferStruct.h
#ifndef PER_MESH_CONSTANT_BUFFER_STRUCT
#define PER_MESH_CONSTANT_BUFFER_STRUCT

#include <gimslib/types.hpp>

struct PerMeshConstantBuffer
{
  gims::f32m4 modelViewMatrix;
};
#endif // PER_MESH_CONSTANT_BUFFER_STRUCT
