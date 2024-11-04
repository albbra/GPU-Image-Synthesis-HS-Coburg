include("../../CreateApp.cmake")
set(SOURCES "./src/main.cpp" 
			"./src/TriangleApp.cpp" 
			"./src/TextureLoader.cpp"
			"./include/TriangleApp.h" 
			"./include/PerFrameConstantsStruct.h" 
			"./include/UIDataStruct.h"
			"./include/VertexStruct.h"
			"./include/TextureLoader.h"
)
set(SHADERS "./shaders/TriangleMesh.hlsl")
create_app(A0MeshViewer "${SOURCES}" "${SHADERS}")

