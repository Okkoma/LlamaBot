cmake_minimum_required(VERSION 3.19)

project(LlamaBot VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# clangd helper
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Configuration spécifique pour Android
if(ANDROID)
    message(STATUS "Configuration Android détectée")
        
    # Désactiver les backends non supportés sur Android
    option(GGML_CUDA "Disable CUDA on Android" OFF)
    option(GGML_METAL "Disable Metal on Android" OFF)
    option(GGML_SYCL "Disable SYCL on Android" OFF)
    # TODO : resolve runtime dlopen error with libomp on android
    option(GGML_OPENMP "Disable OpenMP on Android" OFF)
    
    message(STATUS "Android: Vulkan=${GGML_VULKAN}, OpenCL=${GGML_OPENCL}, BLAS=${GGML_BLAS}, OPENMP=${GGML_OPENMP}")
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
    
    option(BUILD_TEST "Build the tests" OFF)
    option(BUILD_SERVER "Build the server" OFF)
    option(BUILD_DOCS "Build the documentation" OFF)
endif()

option(BUILD_APPLICATION "Build the application" ON)
option(ACTIVE_QML_DEBUG "Enable Qml Debug" ON)

# Recherche de Qt6 globale pour tout le projet (y compris les dépendances)
if(DEFINED ENV{Qt6})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6}")
endif()

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Network 
    Qml 
    Quick 
    QuickWidgets 
    QuickControls2
    Test
    Gui
    Sql
)

if (NOT ANDROID)
    find_package(OpenSSL REQUIRED)
else()
    include(Source/ThirdParty/android_openssl/android_openssl.cmake)
endif()

add_subdirectory (Source/ThirdParty/zlib)
add_library(ZLIB::ZLIB ALIAS zlib)

# skip libpng install
set(SKIP_INSTALL_ALL TRUE)
add_subdirectory (Source/ThirdParty/libpng)
add_library(PNG::PNG ALIAS png_shared)

add_subdirectory (Source/ThirdParty/freetype)
add_library(Freetype::Freetype ALIAS freetype)

add_subdirectory (Source/ThirdParty/poppler)

include_directories (Source/Common Source/Application Source/ThirdParty/llama.cpp/src Source/ThirdParty/poppler/qt6/src)

add_subdirectory (Source/Common)
add_subdirectory (Source/ThirdParty/llama.cpp)

if (BUILD_TEST AND NOT ANDROID)
    enable_testing()
    add_subdirectory (Tests)
endif()

if (BUILD_APPLICATION)
    add_subdirectory (Source/Application)
endif()

if (BUILD_SERVER)
    #add_subdirectory (Source/ServerGrpcChatStore)
    add_subdirectory (Source/ServerHttpRestChatStore)
endif()

# Configuration Doxygen pour la génération de documentation
if (BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen trouvé - configuration de la génération de documentation")
        
        # Configuration de la cible de documentation
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Génération de la documentation avec Doxygen"
            VERBATIM
        )
        
        # Ajout d'une dépendance pour s'assurer que le répertoire de sortie existe
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Docs/doxygen)
    else()
        message(WARNING "Doxygen non trouvé - la cible de documentation ne sera pas disponible")
    endif()
endif()
