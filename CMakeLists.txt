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

    option(BUILD_APPLICATION "Build the application" ON)
    option(BUILD_TEST "Build the tests" ON)
    option(BUILD_DOCS "Build the documentation" ON)
endif()

# Recherche de Qt6 globale pour tout le projet (y compris les dépendances)
if(DEFINED ENV{Qt6})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6}")
endif()
if(DEFINED ENV{QT6})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT6}")
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
)

include_directories (Source/Application Source/ThirdParty/llama.cpp/src Source/ThirdParty/poppler/qt6/src)

add_subdirectory (Source/Common)
add_subdirectory (Source/ThirdParty/llama.cpp)
add_subdirectory (Source/ThirdParty/poppler)

if (BUILD_TEST AND NOT ANDROID)
    enable_testing()
    add_subdirectory (Tests)
endif()

if (BUILD_APPLICATION)
    add_subdirectory (Source/Application)
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