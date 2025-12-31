# LlamaBot

A local conversational assistant using Qt 6 and llama.cpp.

## Overview

LlamaBot is an assistant designed to provide a fluid conversation experience with local language models (LLMs). It notably integrates a RAG (Retrieval Augmented Generation) system allowing you to query your personal documents in complete confidentiality.

## Dependencies

### External Dependencies (to be installed)

The project requires certain system libraries that are not included in the repository:

*   **Qt 6**: Core, Widgets, Network, Qml, Quick, QuickWidgets, QuickControls2.

### Included Dependencies

*   **llama.cpp**: The inference engine is included as a subproject in `Source/ThirdParty/llama.cpp`.
*   **libpoppler**: Included as a subproject in `Source/ThirdParty/poppler`.

#### Dependencies for libpoppler: libfontconfig-dev, libopenjp2.7-dev

## Compilation

### Standard Method

```bash
# Create and enter the build directory
mkdir build
cd build

# Configure the project
cmake ..

# Compile
make -j$(nproc)
```

### Using Build Scripts

Several scripts are provided at the root to automate configuration and compilation according to your hardware:

*   `./build_native.sh`: Standard build with CUDA, OpenCL and BLAS enabled.
*   `./build_native_cpu_only.sh`: Optimized build for CPU only.
*   `./build_native_gpu_cuda.sh`: Specific build for NVIDIA (CUDA).
*   `./build_native_gpu_cl.sh`: Specific build for OpenCL.

For Android, specific scripts are also available (`build_android_*.sh`).

### Compilation Options (GPU Backends)

You can enable GPU acceleration via CMake options in the root file (by default, CUDA and BLAS are enabled if detected):

*   `-DGGML_CUDA=ON`: NVIDIA CUDA support
*   `-DGGML_VULKAN=ON`: Vulkan support
*   `-DGGML_OPENCL=ON`: OpenCL support (AMD/Intel)
*   `-DGGML_BLAS=ON`: CPU acceleration via BLAS

## Features

*   **Modern Interface**: Fluid interface developed in QML with light and dark mode support.
*   **Local Inference**: Full support for `.gguf` format models via llama.cpp.
*   **Ollama Integration**: Ability to connect to a local Ollama server.
*   **RAG System**: Indexing and searching in your documents (PDF, TXT, MD) for answers based on your local knowledge base.
*   **Privacy**: Everything runs locally, no data leaves your machine.

## Currently in Development

*   Multimodal model support.
*   Expand the choice of available models: HuggingFace, manual URL, OCI (see Ramalama project).
*   Conversation grouping/classification system.
*   System tests (Qml Squish).

## License

This project is licensed under the **MIT** License. See the [LICENSE](LICENSE) file for more details.
