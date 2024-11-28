// NodeStruct.h
#ifndef NODE_STRUCT
#define NODE_STRUCT

#include <gimslib/types.hpp>
#include <vector>

/// <summary>
/// Node of the scene graph.
/// </summary>
struct Node
{
  gims::f32m4             transformation = gims::f32m4(1.0f); //! Transformation to parent node.
  std::vector<gims::ui32> meshIndices;  //! Index in the array of meshIndices, i.e., Scene::m_meshes[].
  std::vector<gims::ui32> childIndices; //! Index in the arroy of nodes, i.e.,Scene::m_nodes[].
};

#endif // NODE_STRUCT