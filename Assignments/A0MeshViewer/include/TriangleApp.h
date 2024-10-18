#pragma once
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>

using namespace gims;
class MeshViewer : public gims::DX12App
{
public:
  MeshViewer(const DX12AppConfig config);

  ~MeshViewer();

  virtual void onDraw();
  virtual void onDrawUI();

private:  
  // TODO Implement me!
  gims::ExaminerController m_examinerController;
  f32m4                    m_normalizationTransformation;

  struct UiData
  {
    f32v3 m_backgroundColor  = f32v3(0.25f, 0.25f, 0.25f);
  // TODO Implement me!
  };

  UiData m_uiData;

};
