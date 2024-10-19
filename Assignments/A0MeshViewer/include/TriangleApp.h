// TriangleApp.h
#ifndef TRIANGLE_APP_HEADER
#define TRIANGLE_APP_HEADER

#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <PerFrameConstantsStruct.h>
#include <UIDataStruct.h>

class MeshViewer : public gims::DX12App
{
public:
  MeshViewer(const gims::DX12AppConfig config);

  ~MeshViewer();

  virtual void onDraw();
  virtual void onDrawUI();

private:  
  // ExaminerController zur Kamerasteuerung
  gims::ExaminerController m_examinerController;

  // Transformationen
  gims::f32m4              m_normalizationTransformation;

  // Vertex und Index Buffer
  ComPtr<ID3D12Resource>   m_vertexBuffer;
  ComPtr<ID3D12Resource>   m_indexBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
  D3D12_INDEX_BUFFER_VIEW  m_indexBufferView;

  // Constant Buffer
  ComPtr<ID3D12Resource>   m_constantBuffer;
  PerFrameConstants        m_constantBufferData;
  UINT8*                   m_mappedConstantBuffer;  // Pointer zum gemappten Constant Buffer

  // Anzahl der Indizes für das Draw-Call
  UINT                     m_indexCount;

  UiData                   m_uiData;
};

#endif // TRIANGLE_APP_HEADER
