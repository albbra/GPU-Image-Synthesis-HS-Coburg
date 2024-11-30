// Scene.cpp

#include "Scene.hpp"
#include "ConstantBufferD3D12.hpp"
#include "PerMeshConstantBufferStruct.h"
#include <d3dx12/d3dx12.h>
#include <unordered_map>

void static addToCommandListImpl(Scene& scene, gims::ui32 nodeIdx, gims::f32m4 transformation,
                          const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList,
                          gims::ui32 modelViewRootParameterIdx, gims::ui32 materialConstantsRootParameterIdx,
                          gims::ui32 srvRootParameterIdx)
{

  if (nodeIdx >= scene.getNumberOfNodes())
  {
    return;
  }

  Node        currentNode               = scene.getNode(nodeIdx);
  gims::f32m4 accumulatedTransformation = transformation * currentNode.transformation;

  for (gims::ui32 i = 0; i < currentNode.meshIndices.size(); i++)
  {
    commandList->SetGraphicsRoot32BitConstants(modelViewRootParameterIdx, 16, &accumulatedTransformation, 0);
    scene.getMesh(currentNode.meshIndices[i]).addToCommandList(commandList);
  }

  for (const gims::ui32& nodeIndex : currentNode.childIndices)
  {
    addToCommandListImpl(scene, nodeIndex, accumulatedTransformation, commandList, modelViewRootParameterIdx,
                         materialConstantsRootParameterIdx, srvRootParameterIdx);
  }

  // Assignment 10
  (void)materialConstantsRootParameterIdx;
  (void)srvRootParameterIdx;
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

const gims::ui32 Scene::getNumberOfMeshes() const
{
  return static_cast<gims::ui32>(m_meshes.size());
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
  addToCommandListImpl(*this, 0, transformation, commandList, modelViewRootParameterIdx, materialConstantsRootParameterIdx,
                       srvRootParameterIdx);
}
