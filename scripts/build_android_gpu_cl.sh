#!/bin/bash

# Script de build pour Android
# Assurez-vous d'avoir Qt pour Android installé et configuré

set -e

BUILD_DIR=build-android-cl
NDK_VERSION=26.1.10909125
QT=$QT_ROOT/6.8.1
ANDROID_NDK=$ANDROID_NDK_ROOT

echo "=== Compilation ChatBot pour Android GPU avec backend OpenCL ==="
echo

# Vérifier les variables d'environnement Android
if [ -z "$ANDROID_NDK" ]; then
    echo "! Variable ANDROID_NDK non définie"
    echo "Définissez-la avec: export ANDROID_NDK=/path/to/android-ndk"
    exit 1
fi

if [ -z "$ANDROID_SDK" ]; then
    echo "! Variable ANDROID_SDK non définie"
    echo "Définissez-la avec: export ANDROID_SDK=/path/to/android-sdk"
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

# Configuration pour Android avec OpenCL
echo "Configuration pour Android avec OpenCL..."

$QT/android_arm64_v8a/bin/qt-cmake -S . -GNinja \
	-DANDROID_SDK_ROOT=$ANDROID_SDK \
	-DANDROID_NDK_ROOT=$ANDROID_NDK \
	-DGGML_VULKAN=OFF \
	-DGGML_OPENCL=ON \
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
cmake --build $BUILD_DIR --target apk --config Release -j$(nproc)

if [ $? -ne 0 ]; then
    echo "! Erreur lors de la compilation"
    exit 1
fi

echo "Compilation réussie !"
echo

# Vérifier les fichiers générés
echo "Fichiers générés:"
find $BUILD_DIR -name "*.so" -o -name "*.a" | head -10

echo
echo "ChatBot compilé pour Android avec support OpenCL !"
echo
echo "Pour intégrer dans votre app Android:"
echo "   1. Copiez les .so files dans app/src/main/jniLibs/"
echo "   2. Ajoutez les headers dans votre projet"
echo "   3. Utilisez JNI pour appeler les fonctions C++"
echo
echo "Configuration recommandée pour Android:"
echo "   - Architecture: arm64-v8a (recommandé)"
echo "   - Platform: android-21+ (OpenCL 1.1+)"
echo "   - STL: c++_shared (plus léger)"
echo
echo "Performance attendue sur Android:"
echo "   - GPU Adreno/Mali: 2-5x plus rapide que CPU"
echo "   - Modèles supportés: jusqu'à 3B-7B paramètres"
echo "   - Mémoire GPU: 2-8GB selon l'appareil" 

#echo "=== Installation du build Android ==="
#cmake --install $BUILD_DIR
