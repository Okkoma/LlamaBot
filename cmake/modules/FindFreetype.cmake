# force à construire la libraire fournie dans ThirdParty 
message ("FindFreetype.cmake")

set(Freetype_FOUND TRUE CACHE BOOL "" FORCE)
set(FREETYPE_FOUND TRUE CACHE BOOL "" FORCE)
set(FREETYPE_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/freetype/include" CACHE PATH "" FORCE)
set(FREETYPE_LIBRARIES freetype CACHE STRING "" FORCE)