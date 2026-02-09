# force à construire la libraire fournie dans ThirdParty 
message ("FindZLIB.cmake")

set(zlib_FOUND TRUE CACHE BOOL "" FORCE)
set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
set(ZLIB_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/Source/ThirdParty/zlib
    ${CMAKE_BINARY_DIR}/Source/ThirdParty/zlib
    CACHE PATH "" FORCE
)
set(ZLIB_LIBRARIES ZLIB::ZLIB CACHE STRING "" FORCE)
