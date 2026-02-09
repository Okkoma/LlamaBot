# force à construire la libraire fournie dans ThirdParty 
message ("FindPNG.cmake")

set(png_FOUND TRUE CACHE BOOL "" FORCE)
set(PNG_FOUND TRUE CACHE BOOL "" FORCE)
set(PNG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/libpng" CACHE PATH "" FORCE)
set(PNG_LIBRARIES png_shared CACHE STRING "" FORCE)

