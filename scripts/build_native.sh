#!/bin/bash

set -e

BUILD_DIR=build-native

echo "=== Configuration du build Natif ==="
cmake -S . -B $BUILD_DIR -DGGML_CUDA=ON -DGGML_OPENCL=ON -DGGML_BLAS=ON

echo "=== Construction du build Natif ==="
cmake --build $BUILD_DIR -j$(nproc)

#echo "=== Installation du build Natif ==="
#cmake --install $BUILD_DIR
