// Scene.hpp
#ifndef SCENE_CLASS
#define SCENE_CLASS

#include "MaterialConstantBufferStruct.h"
#include "MaterialStruct.h"
#include "NodeStruct.h"
#include "TriangleMeshD3D12.hpp"
#include <Texture2DD3D12.hpp>
#include <d3d12.h>
#include <gimslib/types.hpp>
#include <vector>

class SceneGraphFactory;

/// <summary>
/// Class that represents a scene graph for D3D12 rendering.
/// </summary>
class Scene
{
public:
  /// <summary>
  /// Default constructor.
  /// </summary>
  Scene() = default;

  /// <summary>
  /// Returns the axis aligned bounding box of the scene.
  /// </summary>
  const AABB& getAABB() const;

  /// <summary>
  /// Nodes are stored in a flat 1D array. This functions returns the Node at the respecitve index.
  /// </summary>
  /// <param name="nodeIdx">Index of the node within the array of nodes.</param>
  /// <returns></returns>
  const Node& getNode(gims::ui32 nodeIdx) const;

  /// <summary>
  /// Nodes are stored in a flat 1D array. This functions returns the Node at the respecitve index.
  /// </summary>
  /// <param name="nodeIdx">Index of the node within the array of nodes.</param>
  /// <returns></returns>
  Node& getNode(gims::ui32 nodeIdx);

  /// <summary>
  /// Returns the total number of nodes.
  /// </summary>
  /// <returns></returns>
  const gims::ui32 getNumberOfNodes() const;

  /// <summary>
  /// Triangle meshes are stored in a 1D array. This functions returns the TriangleMeshD3D12 at the respective index.
  /// </summary>
  /// <param name="materialIdx">Index of the mesh.</param>
  const TriangleMeshD3D12& getMesh(gims::ui32 meshIdx) const;

  /// <summary>
  /// Materials are stored in a 1D array. This function returns the Material at the respective index.
  /// </summary>
  /// <param name="materialIdx">The index of the material</param>
  const Material& getMaterial(gims::ui32 materialIdx) const;

  /// <summary>
  /// Traverse the scene graph and add the draw calls, and all other neccessary commands to the command list.
  /// </summary>
  /// <param name="commandList">The command list to which the commands will be added.</param>
  /// <param name="viewMatrix">The view matrix (or camera matrix).</param>
  /// <param name="modelViewRootParameterIdx">>In your root signature, reserve 16 floats for root constants
  /// which obtain the model view matrix.</param>
  /// <param name="materialConstantsRootParameterIdx">In your root signature, the parameter index of the material
  /// constant buffer.</param>
  /// <param name="srvRootParameterIdx">In your root signature the paramer index of the Shader-Resource-View For the
  /// textures.</param>
  void addToCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList,
                        const gims::f32m4 transformation, gims::ui32 modelViewRootParameterIdx,
                        gims::ui32 materialConstantsRootParameterIdx, gims::ui32 srvRootParameterIdx);

  // Allow the class SceneGraphFactor access to the privatem mebers.
  friend class SceneGraphFactory;

private:
  std::vector<Node>              m_nodes;     //! The nodes of the scene.
  std::vector<TriangleMeshD3D12> m_meshes;    //! Array meshes of the scene.
  AABB                           m_aabb;      //! The axis-aligned bounding box of the scene.
  std::vector<Material>          m_materials; //! Material information for each mesh.
  std::vector<Texture2DD3D12>    m_textures;  //! Array of textures.
};

#endif // SCENE_CLASS
