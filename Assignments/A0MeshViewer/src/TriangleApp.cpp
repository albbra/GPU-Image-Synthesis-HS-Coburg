// TriangleApp.cpp
#include "TriangleApp.h"
#include "PerFrameConstantsStruct.h"
#include <d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <gimslib/sys/Event.hpp>
#include <imgui.h>
#include <iostream>
#include <vector>

using namespace gims;

namespace math
{

f32m4 static getNormalizationTransformation(f32v3 const* const positions, ui32 nPositions)
{
  // Berechne die minimalen und maximalen Eckpunkte des Modells
  f32v3 minPosition = positions[0];
  f32v3 maxPosition = positions[0];

  for (ui32 i = 1; i < nPositions; ++i)
  {
    minPosition = glm::min(minPosition, positions[i]); // Minima pro Achse finden
    maxPosition = glm::max(maxPosition, positions[i]); // Maxima pro Achse finden
  }

  // Berechne den Schwerpunkt des Modells (Mittelpunkt der Bounding Box)
  f32v3 center = (minPosition + maxPosition) * 0.5f;

  // Berechne die Ausdehnung des Modells in jeder Achse (x, y, z)
  f32v3 extents = maxPosition - minPosition;

  // Bestimme die längste Achse der Bounding Box
  f32 maxExtent = glm::max(extents.x, glm::max(extents.y, extents.z));

  // Berechne den Skalierungsfaktor basierend auf der längsten Achse
  f32 scale = 1.0f / maxExtent;

  // Erstelle die Normalisierungstransformation (Translation + Skalierung)
  // Zuerst skalieren, dann in den Ursprung verschieben (Translation um den Schwerpunkt)
  f32m4 normalizationMatrix = glm::scale(f32m4(1.0f), f32v3(scale));  // Skalierung auf [0, 1] Bereich
  normalizationMatrix = glm::translate(normalizationMatrix, -center); // Translation, um den Schwerpunkt zu verschieben

  return normalizationMatrix;
}
} // namespace math

MeshViewer::MeshViewer(const DX12AppConfig config)
    : DX12App(config)
    , m_examinerController(true)
    , m_pipelineState()
    , m_rootSignature()
    , m_vertexBuffer()
    , m_vertexBufferView()
    , m_indexBuffer()
    , m_indexBufferView()
    , m_constantBuffers()
    , m_cbv()
    , m_normalizationTransformation()
    , m_uiData()
    , m_perFrameData()
    , m_view()
    , m_projection()
{
  m_examinerController.setTranslationVector(f32v3(0, 0, 3));

  createRootSignature();
  createPipeline();
  loadMesh();
  createConstantBuffer();
}

void MeshViewer::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
  rootParameters[0].InitAsConstantBufferView(0);

  // Define a static sampler
  CD3DX12_STATIC_SAMPLER_DESC samplerDesc(0);

  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = {};
  descRootSignature.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc,
                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
}

void MeshViewer::createPipeline()
{
  const ComPtr<IDxcBlob> vertexShader =
      compileShader(L"../../../Assignments/A0MeshViewer/shaders/TriangleMesh.hlsl", L"VS_main", L"vs_6_0");

  const ComPtr<IDxcBlob> pixelShader =
      compileShader(L"../../../Assignments/A0MeshViewer/shaders/TriangleMesh.hlsl", L"PS_main", L"ps_6_0");

  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

  psoDesc.InputLayout                      = {inputElementDescs, _countof(inputElementDescs)};
  psoDesc.pRootSignature                   = m_rootSignature.Get();
  psoDesc.VS                               = HLSLCompiler::convert(vertexShader);
  psoDesc.PS                               = HLSLCompiler::convert(pixelShader);
  psoDesc.RasterizerState                  = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                       = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState                = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psoDesc.SampleMask                       = UINT_MAX;
  psoDesc.PrimitiveTopologyType            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                 = 1;
  psoDesc.SampleDesc.Count                 = 1;
  psoDesc.RTVFormats[0]                    = getRenderTarget()->GetDesc().Format;
  psoDesc.DSVFormat                        = getDepthStencil()->GetDesc().Format;
  psoDesc.DepthStencilState.DepthEnable    = FALSE;
  psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_ALWAYS;
  psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable  = FALSE;

  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void MeshViewer::loadMesh()
{
  CograBinaryMeshFile cbm("../../../data/bunny.cbm");

  const f32*   positionsRaw = cbm.getPositionsPtr();
  ui32         numVertices  = cbm.getNumVertices();
  const f32v3* positions    = reinterpret_cast<const f32v3*>(positionsRaw);

  m_normalizationTransformation = math::getNormalizationTransformation(positions, numVertices);

  const f32*   normalsRaw = static_cast<const f32*>(cbm.getAttributePtr(0));
  const f32v3* normals    = reinterpret_cast<const f32v3*>(normalsRaw);

  const f32*   texCoordsRaw = static_cast<const f32*>(cbm.getAttributePtr(1));
  const f32v2* texCoords    = reinterpret_cast<const f32v2*>(texCoordsRaw);

  std::vector<Vertex> vertexBufferCPU;

  vertexBufferCPU.resize(numVertices);

  for (ui32 i = 0; i < numVertices; ++i)
  {
    Vertex nVertex = {};

    nVertex.position = positions[i];

    nVertex.normal = normals[i];

    nVertex.texCoord = texCoords[i];

    vertexBufferCPU[i] = nVertex;
  }

  const ui32* indices    = cbm.getTriangleIndices();
  ui32        nTriangles = cbm.getNumTriangles();

  std::vector<ui32> indexBufferCPU(indices, indices + nTriangles * 3);

  const ui64 vertexBufferCPUSizeInBytes = vertexBufferCPU.size() * sizeof(Vertex);
  const ui64 indexBufferCPUSizeInBytes  = indexBufferCPU.size() * sizeof(ui32);

  UploadHelper uploadBuffer(getDevice(), std::max(vertexBufferCPUSizeInBytes, indexBufferCPUSizeInBytes));

  const CD3DX12_RESOURCE_DESC   vertexBufferDesc      = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferCPUSizeInBytes);
  const CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer));
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = (ui32)vertexBufferCPUSizeInBytes;
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);

  uploadBuffer.uploadBuffer(vertexBufferCPU.data(), m_vertexBuffer, vertexBufferCPUSizeInBytes, getCommandQueue());

  const CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferCPUSizeInBytes);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer));

  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = (ui32)indexBufferCPUSizeInBytes;
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  uploadBuffer.uploadBuffer(indexBufferCPU.data(), m_indexBuffer, indexBufferCPUSizeInBytes, getCommandQueue());
}

void MeshViewer::createConstantBuffer()
{
  f32m4 viewMatrix                          = m_examinerController.getTransformationMatrix();
  m_perFrameData.mvp                        = viewMatrix * m_normalizationTransformation;
  m_perFrameData.ambientColor               = f32v4(0.1f, 0.1f, 0.1f, 1.0f);
  m_perFrameData.diffuseColor               = f32v4(0.8f, 0.8f, 0.8f, 1.0f);
  m_perFrameData.specularColor_and_Exponent = f32v4(1.0f, 1.0f, 1.0f, 32.0f);

  // Calculate the view and projection matrices
  f32v3 cameraPosition = {0.0f, 0.0f, 5.0f};
  f32v3 centerPosition = {0.0f, 0.0f, 0.0f};
  f32v3 upPosition     = {0.0f, 1.0f, 0.0f};
  m_view               = glm::lookAt(glm::vec3(cameraPosition), glm::vec3(centerPosition), glm::vec3(upPosition));

  f32 constexpr fieldOfView = glm::radians(45.0f);
  f32 aspectRatio           = static_cast<f32>(500) / 500;
  f32 nearPlane             = 0.2f;
  f32 farPlane              = 10.0f;
  m_projection              = glm::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);

  m_perFrameData.mvp = m_projection * m_view * glm::mat4(1.0f);
  m_perFrameData.mv  = m_view * glm::mat4(1.0f);

  // Set additional per-frame constants
  m_perFrameData.specularColor_and_Exponent = {1.0f, 1.0f, 1.0f, 32.0f};
  m_perFrameData.ambientColor               = {0.2f, 0.2f, 0.2f, 1.0f};
  m_perFrameData.diffuseColor               = {1.0f, 1.0f, 1.0f, 1.0f};
  m_perFrameData.wireFrameColor             = {1.0f, 0.0f, 0.0f, 1.0f};
  m_perFrameData.flags                      = {0};

  // Determine the number of frames to create buffers for
  const ui32 frameCount = getDX12AppConfig().frameCount;
  m_constantBuffers.resize(frameCount);

  // Create each buffer for each frame
  for (ui32 i = 0; i < frameCount; i++)
  {
    static const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    static const CD3DX12_RESOURCE_DESC   bufferDesc =
        CD3DX12_RESOURCE_DESC::Buffer((sizeof(PerFrameConstants) + 0xff) & ~0xff);

    getDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(&m_constantBuffers[i]));

    // Map and copy data for each frame’s constant buffer
    void* mappedData;
    m_constantBuffers[i]->Map(0, nullptr, &mappedData);
    memcpy(mappedData, &m_perFrameData, sizeof(m_perFrameData));
    m_constantBuffers[i]->Unmap(0, nullptr);
  }

  // Create descriptor heap for CBV
  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.NumDescriptors             = frameCount;
  desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_cbv));

  // Create CBV for each frame
  for (ui32 i = 0; i < frameCount; i++)
  {
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation                  = m_constantBuffers[i]->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes                     = (sizeof(PerFrameConstants) + 0xff) & ~0xff;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(
        m_cbv->GetCPUDescriptorHandleForHeapStart(), i,
        getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    getDevice()->CreateConstantBufferView(&cbvDesc, cbvHandle);
  }
}

MeshViewer::~MeshViewer()
{
}

void MeshViewer::onDraw()
{
  const ui32 frameIndex = getFrameIndex();

  if (!ImGui::GetIO().WantCaptureMouse)
  {
    bool pressed  = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right);
    if (pressed || released)
    {
      bool left = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
      m_examinerController.click(pressed, left == true ? 1 : 2,
                                 ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl),
                                 getNormalizedMouseCoordinates());
    }
    else
    {
      m_examinerController.move(getNormalizedMouseCoordinates());
    }
  }

  // Update transformation matrices for this frame
  f32m4 transformationMatrix = m_examinerController.getTransformationMatrix();
  m_perFrameData.mvp         = m_projection * m_view * transformationMatrix;
  m_perFrameData.mv          = m_view * transformationMatrix;

  // Update the constant buffer for the current frame
  updateConstantBuffer();

  const ComPtr<ID3D12GraphicsCommandList> commandList = getCommandList();
  const CD3DX12_CPU_DESCRIPTOR_HANDLE     rtvHandle   = getRTVHandle();
  const CD3DX12_CPU_DESCRIPTOR_HANDLE     dsvHandle   = getDSVHandle();

  // Set root signature
  commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  // Set the constant buffer directly without using a descriptor table
  const ComPtr<ID3D12Resource>& currentConstantBuffer = m_constantBuffers[frameIndex];
  commandList->SetGraphicsRootConstantBufferView(0, currentConstantBuffer->GetGPUVirtualAddress());

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  const float clearColor[] = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z,
                              1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  commandList->SetPipelineState(m_pipelineState.Get());

  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  commandList->DrawIndexedInstanced(m_indexBufferView.SizeInBytes / sizeof(ui32), 1, 0, 0, 0);
}

void MeshViewer::updateConstantBuffer()
{
  PerFrameConstants             cb                    = m_perFrameData;
  const ComPtr<ID3D12Resource>& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];

  void* mappedData = nullptr;
  currentConstantBuffer->Map(0, nullptr, &mappedData);
  ::memcpy(mappedData, &m_perFrameData, sizeof(m_perFrameData));
  currentConstantBuffer->Unmap(0, nullptr);
}

void MeshViewer::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  // TODO Implement me!
  ImGui::Begin("Information", nullptr, imGuiFlags);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();
  // TODO Implement me!
}
