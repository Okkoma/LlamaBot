#!/bin/bash

set -e

BUILD_DIR=build-native-coverage

echo "=== Configuration du build Natif avec coverage ==="
cmake -S . -B $BUILD_DIR -DGGML_CUDA=ON -DGGML_OPENCL=ON -DGGML_BLAS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"

echo "=== Construction du build Natif avec coverage ==="
cmake --build $BUILD_DIR -j$(nproc) --target Test_Chat Test_LLMServices

./$BUILD_DIR/Tests/Test_Chat
./$BUILD_DIR/Tests/Test_LLMServices

gcov -n ./$BUILD_DIR/Tests/CMakeFiles/Test_Chat.dir/__/Source/Application/ChatImpl.cpp.gcda
gcov -n ./$BUILD_DIR/Tests/CMakeFiles/Test_Chat.dir/__/Source/Application/LLMServices.cpp.gcda
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage
