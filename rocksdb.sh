#!/bin/bash
set -euo pipefail

# ==============================
# Configurazione
# ==============================
NDK_PATH=/opt/android-sdk/ndk/23.1.7779620
ANDROID_API=24
ABI=arm64-v8a
NUM_CORES=$(nproc)

# Directory RocksDB
WORKDIR=/root/mevacoin-android/mevacoin/external/rocksdb
BUILD_DIR=$WORKDIR/build-android
INSTALL_DIR=$WORKDIR/out/$ABI

# ==============================
# Pulizia completa di build precedenti
# ==============================
echo "==> Pulizia build precedente..."
rm -rf "$BUILD_DIR" "$INSTALL_DIR"
mkdir -p "$BUILD_DIR" "$INSTALL_DIR"
cd "$WORKDIR"

# Rimuovo eventuali librerie residue da Linux
find . -type f -name "*.a" -exec rm -f {} \;
find . -type f -name "*.so" -exec rm -f {} \;

# ==============================
# Configurazione CMake per Android
# ==============================
echo "==> Configurazione CMake per Android $ABI..."
cd "$BUILD_DIR"
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=$ABI \
  -DANDROID_PLATFORM=android-$ANDROID_API \
  -DCMAKE_BUILD_TYPE=Release \
  -DROCKSDB_BUILD_SHARED=OFF \
  -DROCKSDB_BUILD_UTILITIES=OFF \
  -DROCKSDB_BUILD_TESTS=OFF \
  -DROCKSDB_BUILD_BENCHMARKS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"

# ==============================
# Compilazione e installazione
# ==============================
echo "==> Compilazione RocksDB..."
make -j"$NUM_CORES"
echo "==> Installazione librerie..."
make install

# ==============================
# Verifica output e architettura
# ==============================
echo "==> Verifica librerie generate..."
ls -l "$INSTALL_DIR/lib"
file "$INSTALL_DIR/lib/librocksdb.a"

echo "==> RocksDB ARM64 pronto in: $INSTALL_DIR"
