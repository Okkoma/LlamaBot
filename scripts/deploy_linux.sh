#!/bin/bash

set -e 

cd $(dirname "$0")/..

INSTALL_DIR=exe
DEPLOY_DIR=tmp/deploy
QT_BASE_DIR=$QT_ROOT/6.10.2
TOOLS_DIR=$(pwd)/tools
QMAKE=$(find $QT_BASE_DIR -name qmake -type f | grep "gcc_64/bin/qmake" | head -n 1)
QT_BIN_DIR=$(dirname $QMAKE)

# clean
rm -Rf $INSTALL_DIR $DEPLOY_DIR
mkdir -p $DEPLOY_DIR

# launch configure/build/install
cmake -S . -B build -DGGML_CUDA=OFF -DGGML_BLAS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix $INSTALL_DIR

# prepare env.vars for linuxdeploy
export PATH=$TOOLS_DIR:$QT_BIN_DIR:$PATH
export QMAKE=$QMAKE
export QML_SOURCES_PATHS="Source/Application/qml"
export LINUXDEPLOY_EXCLUDED_LIBRARIES="libqtiff.so;libqsqloci.so;libqsqlodbc.so;libqsqlpsql.so;libqsqlmimer.so;libqsqlibase.so"
export LINUXDEPLOY_OUTPUT_VERSION="0.1.1"

# launch build AppImage
$TOOLS_DIR/linuxdeploy-x86_64.AppImage \
    --appdir=$DEPLOY_DIR \
    --executable=$INSTALL_DIR/bin/LlamaBot \
    --desktop-file=$INSTALL_DIR/share/applications/llamabot.desktop \
    --icon-file=$INSTALL_DIR/share/icons/hicolor/128x128/apps/llamabot.png \
    --icon-file=$INSTALL_DIR/share/icons/hicolor/256x256/apps/llamabot.png \
    --icon-file=$INSTALL_DIR/share/icons/hicolor/512x512/apps/llamabot.png \
    --plugin=qt \
    --output=appimage

mv LlamaBot*.AppImage tmp

# clean
rm -Rf $DEPLOY_DIR
