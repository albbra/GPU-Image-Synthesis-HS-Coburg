#include "TriangleApp.h"
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

namespace
{
// TODO Implement me!

f32m4 getNormalizationTransformation(f32v3 const* const positions, ui32 nPositions)
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
  f32m4 normalizationMatrix = glm::scale(f32m4(1.0f), f32v3(scale)); // Skalierung auf [0, 1] Bereich
  normalizationMatrix = glm::translate(normalizationMatrix, -center); // Translation, um den Schwerpunkt zu verschieben

  return normalizationMatrix;
}
} // namespace

MeshViewer::MeshViewer(const DX12AppConfig config)
    : DX12App(config)
    , m_examinerController(true)
{
  m_examinerController.setTranslationVector(f32v3(0, 0, 3));
  CograBinaryMeshFile cbm("../../../data/bunny.cbm");
  
  const f32* positionsRaw = cbm.getPositionsPtr();
  ui32 numVertices = cbm.getNumVertices();
  const f32v3* positions = reinterpret_cast<const f32v3*>(positionsRaw);

  m_normalizationTransformation = getNormalizationTransformation(positions, numVertices);

  const ui32* indices = cbm.getTriangleIndices();
  ui32 nTriangles = cbm.getNumTriangles();
}

MeshViewer::~MeshViewer()
{
}

void MeshViewer::onDraw()
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

  // TODO Implement me!
  // Use this to get the transformation Matrix.
  // Of course, skip the (void). That is just to prevent warning, sinces I am not using it here (but you will have to!)
  (void)m_examinerController.getTransformationMatrix();

  const auto commandList = getCommandList();
  const auto rtvHandle   = getRTVHandle();
  const auto dsvHandle   = getDSVHandle();
  // TODO Implement me!

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
  // TODO Implement me!

  const float clearColor[] = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z,
                              1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  // TODO Implement me!
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

// TODO Implement me!
// That is a hell lot of code :-)
// Enjoy!
