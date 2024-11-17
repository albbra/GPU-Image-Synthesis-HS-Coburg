// Scene.cpp

#include "Scene.hpp"
#include <d3dx12/d3dx12.h>
#include <unordered_map>

void static addToCommandListImpl(Scene& scene, gims::ui32 nodeIdx, gims::f32m4 transformation,
                                 const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList,
                                 gims::ui32 modelViewRootParameterIdx, gims::ui32 materialConstantsRootParameterIdx,
                                 gims::ui32 srvRootParameterIdx)
{
  (void)scene;
  (void)nodeIdx;
  (void)transformation;
  (void)commandList;
  (void)modelViewRootParameterIdx;
  (void)materialConstantsRootParameterIdx;
  (void)srvRootParameterIdx;
  // Assignemt 6
  // Assignment 10
}

const Node& Scene::getNode(gims::ui32 nodeIdx) const
{
  return m_nodes[nodeIdx];
}

Node& Scene::getNode(gims::ui32 nodeIdx)
{
  return m_nodes[nodeIdx];
}

const gims::ui32 Scene::getNumberOfNodes() const
{
  return static_cast<gims::ui32>(m_nodes.size());
}

const TriangleMeshD3D12& Scene::getMesh(gims::ui32 meshIdx) const
{
  return m_meshes[meshIdx];
}

const Material& Scene::getMaterial(gims::ui32 materialIdx) const
{
  return m_materials[materialIdx];
}

const AABB& Scene::getAABB() const
{
  return m_aabb;
}

void Scene::addToCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList,
                             const gims::f32m4 transformation, gims::ui32 modelViewRootParameterIdx,
                             gims::ui32 materialConstantsRootParameterIdx, gims::ui32 srvRootParameterIdx)
{
  addToCommandListImpl(*this, 0, transformation, commandList, modelViewRootParameterIdx,
                       materialConstantsRootParameterIdx, srvRootParameterIdx);
}
