#!/bin/bash

# Script de build pour Android
# Assurez-vous d'avoir Qt pour Android installé et configuré

set -e

BUILD_DIR=build-android-cpu
NDK_VERSION=26.1.10909125
QT=$QT_ROOT/6.8.1
ANDROID_NDK=$ANDROID_NDK_ROOT
ANDROID_SDK=$ANDROID_SDK_ROOT

# Vérification des variables d'environnement
if [ -z "$ANDROID_SDK" ]; then
    echo "! ANDROID_SDK n'est pas défini"
    echo "Veuillez définir le chemin vers Android SDK"
    exit 1
fi
if [ -z "$ANDROID_NDK" ]; then
    echo ": ANDROID_NDK n'a pas été trouvé"
    echo "Veuillez installer cette version de ndk"
    exit 1
fi
if [ -z "$QT_ROOT/6.8.1" ]; then
    echo "! \$QT_ROOT/6.8.1 n'a pas été trouvé"
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
echo "Configuration pour Android CPU..."

$QT/android_arm64_v8a/bin/qt-cmake -S . -GNinja \
	-DANDROID_SDK_ROOT=$ANDROID_SDK \
	-DANDROID_NDK_ROOT=$ANDROID_NDK \
	-DGGML_VULKAN=OFF \
	-DGGML_OPENCL=OFF \
	-DGGML_CUDA=OFF \
	-DGGML_METAL=OFF \
	-DGGML_SYCL=OFF \
	-DGGML_BLAS=OFF \
	-DBUILD_SHARED_LIBS=ON \
	-DANDROID_STL=c++_shared \
	-B $BUILD_DIR

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

#echo "=== Installation du build Android ==="
#cmake --install $BUILD_DIR
