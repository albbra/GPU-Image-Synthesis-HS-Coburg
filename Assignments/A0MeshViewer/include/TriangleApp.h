// TriangleApp.h
#ifndef TRIANGLE_APP_HEADER
#define TRIANGLE_APP_HEADER

#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <PerFrameConstantsStruct.h>
#include <UIDataStruct.h>
#include <vector>
#include <VertexStruct.h>

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

  UiData                   m_uiData;

  std::vector<Vertex>	   m_mesh;

  void loadAndStoreMesh();
};

#endif // TRIANGLE_APP_HEADER
