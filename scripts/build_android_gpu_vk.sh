#!/bin/bash

# Script de build pour Android
# Assurez-vous d'avoir Qt pour Android installé et configuré

set -e

BUILD_DIR=build-android-vk
NDK_VERSION=27.2.12479018
QT=$QT_ROOT/6.10.2
ANDROID_NDK=$ANDROID_NDK_ROOT
ANDROID_SDK=$ANDROID_SDK_ROOT

echo "=== Compilation ChatBot pour Android GPU avec backend Vulkan ==="
echo

# Vérification des variables d'environnement
if [ -z "$ANDROID_SDK" ]; then
    echo "! ANDROID_SDK n'est pas défini"
    echo "Veuillez définir le chemin vers Android SDK"
    exit 1
fi
if [ -z "$ANDROID_NDK" ]; then
    echo "! ANDROID_NDK n'a pas été trouvé"
    echo "Veuillez installer cette version de ndk"
    exit 1
fi
if [ -z "$QT_ROOT/6.10.2" ]; then
    echo "! \$QT_ROOT/6.10.2 n'a pas été trouvé"
    echo "Veuillez installer cette version de QT"
    exit 1
fi

echo "QT: $QT"
echo "Android NDK: $NDK_VERSION"
echo "Android SDK: $ANDROID_SDK"
echo

# Nettoyer le build précédent
echo "Nettoyage du build précédent..."
rm -rf $BUILD_DIR
mkdir $BUILD_DIR

# Configuration pour Android avec Vulkan
echo "Configuration pour Android avec Vulkan..."

$QT/android_arm64_v8a/bin/qt-cmake -S . -GNinja \
	-DANDROID_SDK_ROOT=$ANDROID_SDK \
	-DANDROID_NDK_ROOT=$ANDROID_NDK \
	-DGGML_VULKAN=ON \
	-DGGML_OPENCL=OFF \
	-DGGML_CUDA=OFF \
	-DGGML_METAL=OFF \
	-DGGML_SYCL=OFF \
	-DGGML_BLAS=OFF \
	-DBUILD_SHARED_LIBS=ON \
	-DANDROID_STL=c++_shared \
	-B $BUILD_DIR

if [ $? -ne 0 ]; then
    echo "! Erreur lors de la configuration CMake"
    exit 1
fi

echo "Configuration réussie"
echo

# Compilation
echo "Compilation en cours..."
cmake --build $BUILD_DIR --target apk --config Debug -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Erreur lors de la compilation"
    exit 1
fi

echo "Compilation réussie !"
echo

# Vérifier les fichiers générés
echo "Fichiers générés:"
find $BUILD_DIR -name "*.so" -o -name "*.a" | head -10

echo
echo "ChatBot compilé pour Android avec support Vulkan !"
echo
echo "Configuration recommandée pour Android Vulkan:"
echo "   - Architecture: arm64-v8a (recommandé)"
echo "   - Platform: android-24+ (Vulkan 1.0+)"
echo "   - STL: c++_shared (plus léger)"
echo
echo "Performance attendue sur Android avec Vulkan:"
echo "   - GPU Adreno/Mali: 3-8x plus rapide que CPU"
echo "   - Modèles supportés: jusqu'à 7B-13B paramètres"
echo "   - Mémoire GPU: 4-12GB selon l'appareil"
echo "   - Batterie: 20-40% plus efficace qu'OpenCL"

#echo "=== Installation du build Android ==="
#cmake --install $BUILD_DIR
