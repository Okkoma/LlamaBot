# force à construire la libraire fournie dans ThirdParty 
message ("FindBLAS.cmake")

set(blas_FOUND TRUE CACHE BOOL "" FORCE)
set(BLAS_FOUND TRUE CACHE BOOL "" FORCE)
set(BLAS_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/Source/ThirdParty/blis/include CACHE PATH "" FORCE)
set(BLAS_LIBRARIES blis CACHE STRING "" FORCE)
