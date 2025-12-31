cmake_minimum_required(VERSION 3.16)

project(LlamaBot VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# clangd helper
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configuration spécifique pour Android
if(ANDROID)
    message(STATUS "Configuration Android détectée")
        
    # Désactiver les backends non supportés sur Android
    option(GGML_CUDA "Disable CUDA on Android" OFF)
    option(GGML_METAL "Disable Metal on Android" OFF)
    option(GGML_SYCL "Disable SYCL on Android" OFF)
    
    message(STATUS "Android: Vulkan=${GGML_VULKAN}, OpenCL=${GGML_OPENCL}, BLAS=${GGML_BLAS}")
else()
    # Configuration des backends GPU pour Desktop/Linux
    # Décommentez les lignes correspondant à votre GPU

    # Pour NVIDIA GPU (CUDA)
    option(GGML_CUDA "Enable CUDA backend" ON)
    if(GGML_CUDA)
        message(STATUS "CUDA backend enabled")
    endif()

    # Pour AMD GPU (OpenCL)
    option(GGML_OPENCL "Enable OpenCL backend" OFF)
    if(GGML_OPENCL)
        message(STATUS "OpenCL backend enabled")
    endif()

    # Pour Vulkan (multi-GPU)
    option(GGML_VULKAN "Enable Vulkan backend" OFF)
    if(GGML_VULKAN)
        message(STATUS "Vulkan backend enabled")
    endif()

    # Pour Apple Silicon (Metal)
    option(GGML_METAL "Enable Metal backend" OFF)
    if(GGML_METAL)
        message(STATUS "Metal backend enabled")
    endif()

    # Pour Intel GPU (SYCL)
    option(GGML_SYCL "Enable SYCL backend" OFF)
    if(GGML_SYCL)
        message(STATUS "SYCL backend enabled")
    endif()

    # BLAS pour accélération CPU
    option(GGML_BLAS "Enable BLAS acceleration" ON)
    if(GGML_BLAS)
        message(STATUS "BLAS acceleration enabled")
    endif()
endif()

add_subdirectory (Source/ThirdParty/llama.cpp)
add_subdirectory (Source/ThirdParty/poppler)
add_subdirectory (Source/Application)

if (NOT ANDROID)
    enable_testing()
    add_subdirectory (Tests)
endif()

include_directories (Source/Application Source/ThirdParty/llama.cpp/src Source/ThirdParty/poppler/qt6/src)

# ajout des headers vulkan directement dans llama.cpp/ggml/ggml-vulkan
