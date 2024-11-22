// TriangleApp.cpp
#include "TriangleApp.h"
#include "PerFrameConstantsStruct.h"
#include "TextureLoader.h"
#include <d3dx12/d3dx12.h>
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
  // Find the minimum and maximum bounds of the mesh vertices
  f32v3 minPosition = positions[0];
  f32v3 maxPosition = positions[0];

  // Loop to determine the minimum and maximum points along each axis
  for (ui32 i = 1; i < nPositions; ++i)
  {
    minPosition = glm::min(minPosition, positions[i]);
    maxPosition = glm::max(maxPosition, positions[i]);
  }

  // Calculate the center of the model (bounding box midpoint)
  f32v3 center = (minPosition + maxPosition) * 0.5f;

  // Calculate the extent (size along each axis)
  f32v3 extents = maxPosition - minPosition;

  // Find the largest extent, used for uniform scaling
  f32 maxExtent = glm::max(extents.x, glm::max(extents.y, extents.z));

  // Calculate a scale factor to normalize the model size
  f32 scale = 1.0f / maxExtent;

  // Build the transformation matrix with translation and scaling
  f32m4 normalizationMatrix = glm::scale(f32m4(1.0f), f32v3(scale));        // Scale to fit in unit cube
  normalizationMatrix       = glm::translate(normalizationMatrix, -center); // Translate to origin

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
  // Set the camera's initial translation vector
  m_examinerController.setTranslationVector(f32v3(0, 0, 3));

  createRootSignature();
  createPipeline();
  loadMesh();
  loadTexture();
  setStartUIData();
  createConstantBuffer();
}

void MeshViewer::createRootSignature()
{
  // Initialize root parameters for constant buffer and descriptor table
  CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
  rootParameters[0].InitAsConstantBufferView(0);

  // Descriptor table for the texture SRV
  CD3DX12_DESCRIPTOR_RANGE range {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0};
  rootParameters[1].InitAsDescriptorTable(1, &range);

  // Create a sampler descriptor
  CD3DX12_STATIC_SAMPLER_DESC samplerDesc(0);
  samplerDesc.Filter   = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

  // Build the root signature description
  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = {};
  descRootSignature.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc,
                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  // Serialize and create the root signature
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

  const ComPtr<IDxcBlob> vertexShaderWF =
      compileShader(L"../../../Assignments/A0MeshViewer/shaders/TriangleMesh.hlsl", L"VS_WireFrame_main", L"vs_6_0");

  const ComPtr<IDxcBlob> pixelShaderWF =
      compileShader(L"../../../Assignments/A0MeshViewer/shaders/TriangleMesh.hlsl", L"PS_WireFrame_main", L"ps_6_0");

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
  psoDesc.DSVFormat                        = DXGI_FORMAT_D32_FLOAT;
  psoDesc.DepthStencilState.DepthEnable    = TRUE;
  psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable  = FALSE;

  // Create pipeline state for solid rendering without back-face culling
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateWithoutCulling)));

  // Enable back-face culling for solid rendering
  psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

  // Set properties for wireframe rendering
  psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

  // Enable depth bias to prevent Z-fighting with solid geometry
  psoDesc.RasterizerState.DepthBias            = 100;  
  psoDesc.RasterizerState.DepthBiasClamp       = 0.0f; 
  psoDesc.RasterizerState.SlopeScaledDepthBias = -1.0f;

  psoDesc.DepthStencilState.DepthWriteMask       = D3D12_DEPTH_WRITE_MASK_ZERO; // Disable depth writes for overlay
  psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
  psoDesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_SRC_ALPHA;
  psoDesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_INV_SRC_ALPHA;
  psoDesc.BlendState.RenderTarget[0].BlendOp     = D3D12_BLEND_OP_ADD;
  psoDesc.VS                                     = HLSLCompiler::convert(vertexShaderWF);
  psoDesc.PS                                     = HLSLCompiler::convert(pixelShaderWF);

  // Create pipeline state for wireframe rendering
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_wireframePipelineState)));
}

void MeshViewer::loadMesh()
{
  // Load mesh data from a custom Cogra Binary Mesh (CBM) file
  CograBinaryMeshFile cbm("../../../data/bunny.cbm");

  // Retrieve vertex positions and calculate normalization transformation
  const f32*   positionsRaw = cbm.getPositionsPtr();
  ui32         numVertices  = cbm.getNumVertices();
  const f32v3* positions    = reinterpret_cast<const f32v3*>(positionsRaw);

  // Normalize the mesh to fit within a unit cube centered at the origin
  m_normalizationTransformation = math::getNormalizationTransformation(positions, numVertices);

  // Load vertex attributes: normals and texture coordinates
  const f32*   normalsRaw = static_cast<const f32*>(cbm.getAttributePtr(0));
  const f32v3* normals    = reinterpret_cast<const f32v3*>(normalsRaw);

  const f32*   texCoordsRaw = static_cast<const f32*>(cbm.getAttributePtr(1));
  const f32v2* texCoords    = reinterpret_cast<const f32v2*>(texCoordsRaw);

  // Prepare the CPU-side vertex buffer to transfer to GPU
  std::vector<Vertex> vertexBufferCPU(numVertices);

  // Populate vertex buffer with position, normal, and texture coordinate data
  for (ui32 i = 0; i < numVertices; ++i)
  {
    Vertex nVertex     = {};
    nVertex.position   = positions[i];
    nVertex.normal     = normals[i];
    nVertex.texCoord   = texCoords[i];
    vertexBufferCPU[i] = nVertex;
  }

  // Retrieve triangle indices for the mesh and prepare an index buffer
  const ui32*       indices    = cbm.getTriangleIndices();
  ui32              nTriangles = cbm.getNumTriangles();
  std::vector<ui32> indexBufferCPU(indices, indices + nTriangles * 3);

  // Calculate sizes in bytes for the vertex and index buffers
  const ui64 vertexBufferCPUSizeInBytes = vertexBufferCPU.size() * sizeof(Vertex);
  const ui64 indexBufferCPUSizeInBytes  = indexBufferCPU.size() * sizeof(ui32);

  // Initialize an upload helper to facilitate buffer uploads to GPU
  UploadHelper uploadBuffer(getDevice(), std::max(vertexBufferCPUSizeInBytes, indexBufferCPUSizeInBytes));

  // Describe and create the GPU vertex buffer
  const CD3DX12_RESOURCE_DESC   vertexBufferDesc      = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferCPUSizeInBytes);
  const CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

  // Set up the vertex buffer view for the GPU
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = static_cast<ui32>(vertexBufferCPUSizeInBytes);
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);

  // Upload vertex buffer data to the GPU
  uploadBuffer.uploadBuffer(vertexBufferCPU.data(), m_vertexBuffer, vertexBufferCPUSizeInBytes, getCommandQueue());

  // Describe and create the GPU index buffer
  const CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferCPUSizeInBytes);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer));

  // Set up the index buffer view for the GPU
  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = static_cast<ui32>(indexBufferCPUSizeInBytes);
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  // Upload index buffer data to the GPU
  uploadBuffer.uploadBuffer(indexBufferCPU.data(), m_indexBuffer, indexBufferCPUSizeInBytes, getCommandQueue());
}

void MeshViewer::loadTexture()
{
  // Load texture image data from a PNG file
  TextureLoader bunnyTexture("../../../data/bunny.png");

  // Define the texture resource description
  D3D12_RESOURCE_DESC textureDesc = {};
  textureDesc.MipLevels           = 1;
  textureDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.Width               = bunnyTexture.getWidth();
  textureDesc.Height              = bunnyTexture.getHeight();
  textureDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
  textureDesc.DepthOrArraySize    = 1;
  textureDesc.SampleDesc.Count    = 1;
  textureDesc.SampleDesc.Quality  = 0;
  textureDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  // Create a committed resource for the texture on the GPU
  const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  throwIfFailed(getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc,
                                                     D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_texture)));

  // Calculate the required upload size for the texture and create an upload helper
  UploadHelper uploadHelper(getDevice(), GetRequiredIntermediateSize(m_texture.Get(), 0, 1));

  // Upload texture data to the GPU
  uploadHelper.uploadTexture(bunnyTexture.getData(), m_texture, bunnyTexture.getWidth(), bunnyTexture.getHeight(),
                             getCommandQueue());

  // Create a shader resource view (SRV) descriptor heap for the texture
  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors             = 1;
  desc.NodeMask                   = 0;
  desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  throwIfFailed(getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srv)));

  // Define the SRV properties for the texture and create the SRV
  D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
  shaderResourceViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  shaderResourceViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  shaderResourceViewDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
  shaderResourceViewDesc.Texture2D.MipLevels             = 1;
  shaderResourceViewDesc.Texture2D.MostDetailedMip       = 0;
  shaderResourceViewDesc.Texture2D.ResourceMinLODClamp   = 0.0f;

  // Create the shader resource view on the descriptor heap
  getDevice()->CreateShaderResourceView(m_texture.Get(), &shaderResourceViewDesc,
                                        m_srv->GetCPUDescriptorHandleForHeapStart());
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

  m_projection = getScaledProjectionMatrix();

  m_perFrameData.mvp = m_projection * m_view * glm::mat4(1.0f);
  m_perFrameData.mv  = m_view * glm::mat4(1.0f);

  // Set additional per-frame constants
  m_perFrameData.specularColor_and_Exponent = {1.0f, 1.0f, 1.0f, 32.0f};
  m_perFrameData.ambientColor               = {0.2f, 0.2f, 0.2f, 1.0f};
  m_perFrameData.diffuseColor               = {0.9f, 0.9f, 0.9f, 1.0f};
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

void MeshViewer::setStartUIData()
{
  // Setting start values not in the Header
  m_uiData.backgroundColor  = gims::f32v3(0.25f, 0.25f, 0.25f);
  m_uiData.backFaceCulling  = false;
  m_uiData.overlayWireframe = false;
  m_uiData.wireframeColor   = gims::f32v3(0.0f, 0.0f, 0.0f);
  m_uiData.twoSidedLighting = false;
  m_uiData.useTexture       = false;
  m_uiData.ambientColor     = gims::f32v3(0.0f, 0.0f, 0.0f);
  m_uiData.diffuseColor     = gims::f32v3(1.0f, 1.0f, 1.0f);
  m_uiData.specularColor    = gims::f32v3(1.0f, 1.0f, 1.0f);
  m_uiData.exponent         = 128.0f;
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

  // Update the PerFrameData
  setPerFrameData(transformationMatrix);

  // Update the constant buffer for the current frame
  updateConstantBuffer();

  const ComPtr<ID3D12GraphicsCommandList> commandList = getCommandList();
  const CD3DX12_CPU_DESCRIPTOR_HANDLE     rtvHandle   = getRTVHandle();
  const CD3DX12_CPU_DESCRIPTOR_HANDLE     dsvHandle   = getDSVHandle();

  // Set root signature
  commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  commandList->SetDescriptorHeaps(1, m_srv.GetAddressOf());
  commandList->SetGraphicsRootDescriptorTable(1, m_srv->GetGPUDescriptorHandleForHeapStart());

  // Set the constant buffer directly without using a descriptor table
  const ComPtr<ID3D12Resource>& currentConstantBuffer = m_constantBuffers[frameIndex];
  commandList->SetGraphicsRootConstantBufferView(0, currentConstantBuffer->GetGPUVirtualAddress());

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  const float clearColor[] = {m_uiData.backgroundColor.x, m_uiData.backgroundColor.y, m_uiData.backgroundColor.z, 1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  // First Pass: Render base geometry
  if (m_uiData.backFaceCulling)
  {
    commandList->SetPipelineState(m_pipelineState.Get());
  }
  else
  {
    commandList->SetPipelineState(m_pipelineStateWithoutCulling.Get());
  }

  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->DrawIndexedInstanced(m_indexBufferView.SizeInBytes / sizeof(ui32), 1, 0, 0, 0);

  // Second Pass: Render wireframe overlay if enabled
  if (m_uiData.overlayWireframe)
  {
    commandList->SetPipelineState(m_wireframePipelineState.Get());
    commandList->DrawIndexedInstanced(m_indexBufferView.SizeInBytes / sizeof(ui32), 1, 0, 0, 0);
  }
}

void MeshViewer::setPerFrameData(gims::f32m4& newTransformationMatrix)
{
  m_projection = getScaledProjectionMatrix();

  m_perFrameData.mvp                        = m_projection * m_view * newTransformationMatrix;
  m_perFrameData.mv                         = m_view * newTransformationMatrix;
  m_perFrameData.specularColor_and_Exponent = {m_uiData.specularColor, m_uiData.exponent};
  m_perFrameData.ambientColor               = {m_uiData.ambientColor, 1.0f};
  m_perFrameData.diffuseColor               = {m_uiData.diffuseColor, 1.0f};
  m_perFrameData.wireFrameColor             = {m_uiData.wireframeColor, 1.0f};

  // Clear bit and set 1 if true
  m_perFrameData.flags = (m_perFrameData.flags & ~0x1) | (m_uiData.twoSidedLighting ? 0x1 : 0);
  m_perFrameData.flags = (m_perFrameData.flags & ~0x2) | (m_uiData.useTexture ? 0x2 : 0);
}

void MeshViewer::updateConstantBuffer()
{
  // get the Constant Buffer of the current Frame
  const ComPtr<ID3D12Resource>& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];

  // Update Constant Buffer
  void* mappedData = nullptr;
  currentConstantBuffer->Map(0, nullptr, &mappedData);
  ::memcpy(mappedData, &m_perFrameData, sizeof(m_perFrameData));
  currentConstantBuffer->Unmap(0, nullptr);
}

void MeshViewer::onDrawUI()
{
  const ImGuiWindowFlags imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  ImGui::Begin("Configuration", nullptr, imGuiFlags);
  ImGui::ColorEdit3("Background Color", reinterpret_cast<float*>(&m_uiData.backgroundColor));
  ImGui::Checkbox("Back-Face Culling", &m_uiData.backFaceCulling);
  ImGui::Checkbox("Overlay Wireframe", &m_uiData.overlayWireframe);
  ImGui::ColorEdit3("Wireframe Color", reinterpret_cast<float*>(&m_uiData.wireframeColor));
  ImGui::Checkbox("Two-Sided Lighting", &m_uiData.twoSidedLighting);
  ImGui::Checkbox("Use Texture", &m_uiData.useTexture);
  ImGui::ColorEdit3("Ambient", reinterpret_cast<float*>(&m_uiData.ambientColor));
  ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&m_uiData.diffuseColor));
  ImGui::ColorEdit3("Specular", reinterpret_cast<float*>(&m_uiData.specularColor));
  ImGui::SliderFloat("Exponent", &m_uiData.exponent, 0.0f, 1024.0f);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();
}

gims::f32m4 MeshViewer::getScaledProjectionMatrix()
{
  f32 constexpr fieldOfView = glm::radians(45.0f);
  f32 const nearPlane       = 0.2f;
  f32 const farPlane        = 10.0f;
  return glm::perspectiveFovRH_NO<f32>(fieldOfView, static_cast<f32>(getWidth()), static_cast<f32>(getHeight()),
                                       nearPlane, farPlane);
}
