#!/bin/bash

BUILD_DIR=build-native-opencl

echo "=== Compilation ChatBot avec support OpenCL ==="
echo "Alternative à CUDA pour accélération GPU"
echo

# Installer OpenCL si nécessaire
echo "Vérification d'OpenCL..."
if ! command -v clinfo &> /dev/null; then
    echo "Installation d'OpenCL..."
    sudo apt update
    sudo apt install -y clinfo ocl-icd-opencl-dev
fi

# Nettoyer le build précédent
echo "Nettoyage du build précédent..."
rm -rf $BUILD_DIR
mkdir $BUILD_DIR

# Configuration avec OpenCL
echo "Configuration avec OpenCL..."
cmake -B $BUILD_DIR \
    -DCMAKE_BUILD_TYPE=Release \
    -DGGML_CUDA=OFF \
    -DGGML_OPENCL=ON \
    -DGGML_BLAS=ON \
    -DGGML_VULKAN=OFF \
    -DGGML_METAL=OFF \
    -DGGML_SYCL=OFF

if [ $? -ne 0 ]; then
    echo "! Erreur lors de la configuration CMake"
    exit 1
fi

echo "Configuration réussie"
echo

# Compilation
echo "Compilation en cours..."
cmake --build $BUILD_DIR --config Release -j$(nproc)

if [ $? -ne 0 ]; then
    echo "! Erreur lors de la compilation"
    exit 1
fi

echo "Compilation réussie !"
echo

# Vérifier que l'exécutable a été créé
if [ -f "$BUILD_DIR/bin/ChatBot" ]; then
    echo "ChatBot compilé avec succès avec support OpenCL !"
    echo "Exécutable: $BUILD_DIR/bin/ChatBot"
    echo
    echo "Pour tester le GPU:"
    echo "   ./$BUILD_DIR/bin/ChatBot"
    echo
    echo "Pour vérifier les backends disponibles:"
    echo "   Regardez les logs au démarrage de l'application"
else
    echo "! Exécutable non trouvé dans $BUILD_DIR/bin/"
    echo "Vérifiez le nom de l'exécutable dans votre projet"
fi 