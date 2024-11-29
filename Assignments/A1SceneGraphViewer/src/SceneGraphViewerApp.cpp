// SceneGraphViewerApp.cpp

#include "SceneGraphViewerApp.hpp"
#include "ConstantBufferStruct.h"
#include "PerMeshConstantBufferStruct.h"
#include "SceneFactory.hpp"
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

SceneGraphViewerApp::SceneGraphViewerApp(const DX12AppConfig config, const std::filesystem::path pathToScene)
    : DX12App(config)
    , m_examinerController(true)
    , m_scene(SceneGraphFactory::createFromAssImpScene(pathToScene, getDevice(), getCommandQueue()))
{
  m_examinerController.setTranslationVector(gims::f32v3(0, -0.25f, 1.5));
  createRootSignature();
  createSceneConstantBuffer();
  createScenePerMeshConstantBuffer();
  createPipeline();
}

void SceneGraphViewerApp::onDraw()
{
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

  const ComPtr<ID3D12GraphicsCommandList> commandList = getCommandList();
  const CD3DX12_CPU_DESCRIPTOR_HANDLE     rtvHandle   = getRTVHandle();
  const CD3DX12_CPU_DESCRIPTOR_HANDLE     dsvHandle   = getDSVHandle();

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  const float clearColor[] = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z,
                              1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  drawScene(commandList);
}

void SceneGraphViewerApp::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  ImGui::Begin("Information", nullptr, imGuiFlags);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();
  ImGui::Begin("Configuration", nullptr, imGuiFlags);
  ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor[0]);
  ImGui::End();
}

void SceneGraphViewerApp::createRootSignature()
{
  // Define root parameters for each of the constant buffers and descriptor table
  CD3DX12_ROOT_PARAMETER rootParameters[4] = {};

  // Initialize as constant buffer views (cbv) for b0, b1, and b2
  rootParameters[0].InitAsConstantBufferView(0); // PerFrameConstants (b0)
  rootParameters[1].InitAsConstantBufferView(1); // PerMeshConstants (b1)
  rootParameters[2].InitAsConstantBufferView(2); // Material (b2)

  // Descriptor table for the texture SRVs (t0-t4)
  CD3DX12_DESCRIPTOR_RANGE srvRange = {};
  srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0); // 5 textures starting at t0
  rootParameters[3].InitAsDescriptorTable(1, &srvRange);

  // Initialize the sampler (s0)
  CD3DX12_STATIC_SAMPLER_DESC samplerDesc(0);
  samplerDesc.Filter   = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

  // Build the root signature description
  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = {};
  descRootSignature.Init(_countof(rootParameters),                                    // Number of root parameters
                         rootParameters,                                              // Array of root parameters
                         1,                                                           // Number of static samplers
                         &samplerDesc,                                                // Pointer to the static sampler
                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT // Flags
  );

  // Serialize and create the root signature
  ComPtr<ID3DBlob> rootBlob, errorBlob;
  HRESULT hr = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  if (FAILED(hr))
  {
    // Handle serialization errors (debug messages or logging)
    if (errorBlob)
    {
      OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
    }
    throw std::runtime_error("Failed to serialize root signature.");
  }

  // Create the root signature on the device
  hr = getDevice()->CreateRootSignature(0,                             // Node mask
                                        rootBlob->GetBufferPointer(),  // Root signature blob
                                        rootBlob->GetBufferSize(),     // Blob size
                                        IID_PPV_ARGS(&m_rootSignature) // Output root signature
  );

  if (FAILED(hr))
  {
    throw std::runtime_error("Failed to create root signature.");
  }
}

void SceneGraphViewerApp::createPipeline()
{
  waitForGPU();
  const std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs = TriangleMeshD3D12::getInputElementDescriptors();

  const ComPtr<IDxcBlob> vertexShader =
      compileShader(L"../../../Assignments/A1SceneGraphViewer/Shaders/TriangleMesh.hlsl", L"VS_main", L"vs_6_0");
  const ComPtr<IDxcBlob> pixelShader =
      compileShader(L"../../../Assignments/A1SceneGraphViewer/Shaders/TriangleMesh.hlsl", L"PS_main", L"ps_6_0");

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {inputElementDescs.data(), (ui32)inputElementDescs.size()};
  psoDesc.pRootSignature                     = m_rootSignature.Get();
  psoDesc.VS                                 = HLSLCompiler::convert(vertexShader);
  psoDesc.PS                                 = HLSLCompiler::convert(pixelShader);
  psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode           = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DSVFormat                          = getDX12AppConfig().depthBufferFormat;
  psoDesc.DepthStencilState.DepthEnable      = TRUE;
  psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable    = FALSE;
  psoDesc.SampleMask                         = UINT_MAX;
  psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                   = 1;
  psoDesc.RTVFormats[0]                      = getDX12AppConfig().renderTargetFormat;
  psoDesc.SampleDesc.Count                   = 1;
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void SceneGraphViewerApp::drawScene(const ComPtr<ID3D12GraphicsCommandList>& cmdLst)
{
  
  const auto cb           = m_constantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  const auto mcb           = m_perMeshConstantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  const auto cameraMatrix = m_examinerController.getTransformationMatrix();

  updateSceneConstantBuffer();
  


  // Assignment 6

  cmdLst->SetPipelineState(m_pipelineState.Get());

  cmdLst->SetGraphicsRootSignature(m_rootSignature.Get());
  cmdLst->SetGraphicsRootConstantBufferView(0, cb);

  m_scene.addToCommandList(cmdLst, cameraMatrix, 1, 2, 3);

  gims::ui32 meshNumber = 1;//m_scene.getNumberOfMeshes();
  for (gims::ui32 meshIndex = 0; meshIndex < meshNumber; meshIndex++)
  {
    const auto modelMatrix = m_scene.getMesh(meshIndex)
                                 .getAABB()
                                 .getNormalizationTransformation();
    const auto mv = cameraMatrix * modelMatrix;
    cmdLst->SetGraphicsRootConstantBufferView(1, mcb);
    updatePerMeshConstantBuffer(mv);
    m_scene.getMesh(meshIndex).addToCommandList(cmdLst);
  }
}

void SceneGraphViewerApp::createSceneConstantBuffer()
{
  const ConstantBuffer cb         = {};
  const gims::ui64     frameCount = getDX12AppConfig().frameCount;
  m_constantBuffers.resize(frameCount);
  for (gims::ui32 i = 0; i < frameCount; i++)
  {
    m_constantBuffers[i] = ConstantBufferD3D12(cb, getDevice());
  }
}

void SceneGraphViewerApp::createScenePerMeshConstantBuffer()
{
  const PerMeshConstantBuffer mcb         = {};
  const gims::ui64     frameCount = getDX12AppConfig().frameCount;
  m_perMeshConstantBuffers.resize(frameCount);
  for (gims::ui32 i = 0; i < frameCount; i++)
  {
    m_perMeshConstantBuffers[i] = ConstantBufferD3D12(mcb, getDevice());
  }
}

void SceneGraphViewerApp::updateSceneConstantBuffer()
{
  ConstantBuffer cb   = {};
  cb.projectionMatrix = glm::perspectiveFovLH_ZO<gims::f32>(glm::radians(45.0f), (gims::f32)getWidth(),
                                                            (gims::f32)getHeight(), 1.0f / 256.0f, 256.0f);
  m_constantBuffers[getFrameIndex()].upload(&cb);
}

void SceneGraphViewerApp::updatePerMeshConstantBuffer(gims::f32m4 mv)
{
  PerMeshConstantBuffer mcb = {};
  mcb.modelViewMatrix = mv;

  m_perMeshConstantBuffers[getFrameIndex()].upload(&mcb);
}
