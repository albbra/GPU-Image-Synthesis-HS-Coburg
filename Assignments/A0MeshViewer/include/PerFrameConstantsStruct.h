// PerFrameConstantsStruct.h
#ifndef PER_FRAME_CONSTANTS_STRUCT
#define PER_FRAME_CONSTANTS_STRUCT

#include <gimslib/types.hpp>

struct PerFrameConstants
{
  gims::f32m4 mvp;
  gims::f32m4 mv;
  gims::f32v4 specularColor_and_Exponent;
  gims::f32v4 ambientColor;
  gims::f32v4 diffuseColor;
  gims::f32v4 wireFrameColor;
  gims::ui32  flags;
};

#endif // PER_FRAME_CONSTANTS_STRUCT
