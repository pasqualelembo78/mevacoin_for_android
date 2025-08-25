#!/bin/bash
set -euo pipefail

# ==============================
# CONFIGURAZIONE
# ==============================
ABI=arm64-v8a
ANDROID_API=24
NUM_CORES=$(nproc)

WORKDIR=$HOME/mevacoin-android
REPO_URL="https://github.com/pasqualelembo78/mevacoin.git"
SDK_DIR=/opt/android-sdk
ANDROID_NDK_VERSION="23.1.7779620"
ANDROID_NDK_PATH=$SDK_DIR/ndk/$ANDROID_NDK_VERSION

# Boost Android
BOOST_ANDROID_ROOT=/opt/boost/build/out/$ABI
BOOST_GENERIC_INCLUDE=/opt/boost/boost_1_85_0
BOOST_MERGED=/opt/boost/android-merged
mkdir -p "$BOOST_MERGED/include" "$BOOST_MERGED/lib"
cp -r $BOOST_GENERIC_INCLUDE/* $BOOST_MERGED/include/
cp -r $BOOST_ANDROID_ROOT/include/* $BOOST_MERGED/include/
cp -r $BOOST_ANDROID_ROOT/lib/* $BOOST_MERGED/lib/
export BOOST_ROOT=$BOOST_MERGED
export BOOST_INCLUDEDIR=$BOOST_MERGED/include
export BOOST_LIBRARYDIR=$BOOST_MERGED/lib

# RocksDB Android
ROCKSDB_ROOT=$WORKDIR/mevacoin/external/rocksdb/out/$ABI
ROCKSDB_INCLUDE=$ROCKSDB_ROOT/include
ROCKSDB_LIB=$ROCKSDB_ROOT/lib/librocksdb.a

# ==============================
# CLONA MEVACOIN
# ==============================
mkdir -p "$WORKDIR"
cd "$WORKDIR"

echo "==> Clonaggio repository Mevacoin..."
if [ ! -d "mevacoin" ]; then
    git clone "$REPO_URL"
else
    cd mevacoin
    git pull
    cd ..
fi
cd mevacoin

# ==============================
# PULIZIA BUILD PRECEDENTE
# ==============================
rm -rf build
mkdir -p build
cd build

# ==============================
# CONFIGURAZIONE CMAKE PER CROSS-COMPILAZIONE
# ==============================
echo "==> Configurazione CMake per Android $ABI..."
cmake .. \
  -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_PATH/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=$ABI \
  -DANDROID_PLATFORM=android-$ANDROID_API \
  -DBOOST_ROOT="$BOOST_ROOT" \
  -DBOOST_INCLUDEDIR="$BOOST_INCLUDEDIR" \
  -DBOOST_LIBRARYDIR="$BOOST_LIBRARYDIR" \
  -DRocksDB_INCLUDE_DIR="$ROCKSDB_INCLUDE" \
  -DRocksDB_LIBRARY="$ROCKSDB_LIB" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_CXX_FLAGS="-I$BOOST_INCLUDEDIR -I$ROCKSDB_INCLUDE -I$ANDROID_NDK_PATH/sources/android/cpufeatures -D__ANDROID__ -DANDROID -Wno-error -Wno-shadow -Wno-deprecated-copy" \
  -DCMAKE_EXE_LINKER_FLAGS="-L$ROCKSDB_ROOT/lib -lrocksdb -pthread -lm" \
  -DFAIL_ON_WARNINGS=OFF

# ==============================
# COMPILAZIONE BINARI
# ==============================
echo "==> Compilazione binari Mevacoin..."
make -j "$NUM_CORES"

# ==============================
# PACCHETTO BINARI
# ==============================
echo "==> Creazione cartella output..."
OUTPUT_DIR="$WORKDIR/output"
mkdir -p "$OUTPUT_DIR"

# Copia solo i binari eseguibili dalla build
find src -maxdepth 1 -type f -executable -exec cp {} "$OUTPUT_DIR/" \;

echo "==> Build completata! Binari disponibili in: $OUTPUT_DIR"
ls -l "$OUTPUT_DIR"
