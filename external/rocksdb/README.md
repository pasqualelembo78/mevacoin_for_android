#!/bin/bash
set -e

# Variabili Android SDK/NDK
export ANDROID_SDK_ROOT=/root/Android/Sdk
export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/25.2.9519653
export PATH=$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/platform-tools:$PATH

# Cartella di build
BUILD_DIR=$(pwd)/build-android
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configurazione CMake per Android
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_TESTS=OFF \
  -DWITH_TOOLS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON

# Compilazione
make -j$(nproc)

echo "=== Build completata! ==="
echo "La libreria librocksdb.a si trova in: $BUILD_DIR"
