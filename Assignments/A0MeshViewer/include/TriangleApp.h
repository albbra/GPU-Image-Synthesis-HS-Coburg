// TriangleApp.h
#ifndef TRIANGLE_APP_HEADER
#define TRIANGLE_APP_HEADER

#include <PerFrameConstantsStruct.h>
#include <UIDataStruct.h>
#include <VertexStruct.h>
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <vector>

class MeshViewer : public gims::DX12App
{
public:
  MeshViewer(const gims::DX12AppConfig config);

  ~MeshViewer();

  virtual void onDraw();
  virtual void onDrawUI();

private:
  gims::ExaminerController    m_examinerController;
  ComPtr<ID3D12PipelineState> m_pipelineState;
  ComPtr<ID3D12PipelineState> m_pipelineStateWithoutCulling;
  ComPtr<ID3D12PipelineState> m_wireframePipelineState;
  ComPtr<ID3D12RootSignature> m_rootSignature;

  ComPtr<ID3D12Resource>   m_vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

  ComPtr<ID3D12Resource>  m_indexBuffer;
  D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

  std::vector<ComPtr<ID3D12Resource>> m_constantBuffers;
  ComPtr<ID3D12DescriptorHeap>        m_cbv;

  ComPtr<ID3D12Resource>       m_texture;
  ComPtr<ID3D12DescriptorHeap> m_srv;

  gims::f32m4 m_normalizationTransformation;

  UiData m_uiData;

  PerFrameConstants m_perFrameData;

  gims::f32m4 m_view;
  gims::f32m4 m_projection;

  void        createRootSignature();
  void        createPipeline();
  void        loadMesh();
  void        loadTexture();
  void        setStartUIData();
  void        setPerFrameData(gims::f32m4& newTransformationMatrix);
  void        createConstantBuffer();
  void        updateConstantBuffer();
  gims::f32m4 getScaledProjectionMatrix();
};

#endif // TRIANGLE_APP_HEADER
