// VertexStruct.h
#ifndef VERTEX_STRUCT
#define VERTEX_STRUCT

#include <gimslib/types.hpp>

struct Vertex
{
  gims::f32v3 position;
  gims::f32v3 normal;
  gims::f32v2 texCoord;
};

#endif // VERTEX_STRUCT
