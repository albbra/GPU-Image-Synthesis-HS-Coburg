// LightStruct.h
#ifndef LIGHT_STRUCT
#define LIGHT_STRUCT

#include <gimslib/types.hpp>

struct Light
{
	gims::f32v3 lightPosition = gims::f32v3(0.0f, 0.0f, 0.0f);
	gims::f32   pad1 = gims::f32(0.0f);
	gims::f32v3 lightColor = gims::f32v3(1.0f, 1.0f, 1.0f);
	gims::f32   lightIntensity = gims::f32(1.0f);
	gims::f32   pad2 = gims::f32(0.0f);
	gims::f32v3 pad3 = gims::f32v3(0.0f, 0.0f, 0.0f);
	gims::f32v4 pad4 = gims::f32v4(0.0f, 0.0f, 0.0f, 0.0f);
};

#endif //LIGHT_STRUCT