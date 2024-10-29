#ifndef UI_DATA_STRUCT
#define UI_DATA_STRUCT

#include <gimslib/types.hpp>

struct UiData
{
  gims::f32v3 backgroundColor;
  bool        backFaceCulling;
  bool        overlayWireframe;
  gims::f32v3 wireframeColor;
  bool        twoSidedLighting;
  bool        useTexture;
  gims::f32v3 ambientColor;
  gims::f32v3 diffuseColor;
  gims::f32v3 specularColor;
  gims::f32   exponent;
};

#endif // UI_DATA_STRUCT