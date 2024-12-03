// UiDataStruct.h
#ifndef USER_INTERFACE_DATA_STRUCT
#define USER_INTERFACE_DATA_STRUCT

#include <gimslib/types.hpp>

struct UiData
{
  gims::f32v3 backgroundColor            = gims::f32v3(0.25f, 0.25f, 0.25f);
  gims::ui32  numberOfNodes              = gims::ui32(0);
  gims::ui32  numberOfMeshes             = gims::ui32(0);
  gims::ui32  numberOfMaterials          = gims::ui32(0);
  gims::ui32  numberOfTextures           = gims::ui32(0);
  gims::f32v3 sceneLowerleftAABBPosition = gims::f32v3(0.0f, 0.0f, 0.0f);
  gims::f32v3 sceneTopRightAABBPosition  = gims::f32v3(0.0f, 0.0f, 0.0f);
};
#endif // USER_INTERFACE_DATA_STRUCT
