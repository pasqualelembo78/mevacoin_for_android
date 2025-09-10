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
SDK_DIR=/root/Android/Sdk
ANDROID_NDK_VERSION="25.2.9519653"
ANDROID_NDK_PATH=$SDK_DIR/ndk/$ANDROID_NDK_VERSION

# ==============================
# BOOST ANDROID
# ==============================
BOOST_ANDROID_ROOT=/opt/boost/build/out/$ABI
BOOST_MERGED=/opt/boost/android-merged
mkdir -p "$BOOST_MERGED/include" "$BOOST_MERGED/lib"

# Copia boost_1_85_0 se esiste
if [ -d /opt/boost/boost_1_85_0 ]; then
    echo "==> Copiando boost_1_85_0..."
    cp -r /opt/boost/boost_1_85_0/* "$BOOST_MERGED/include/" || true
fi

# Copia build Android Boost
cp -r $BOOST_ANDROID_ROOT/include/* "$BOOST_MERGED/include/" || true
cp -r $BOOST_ANDROID_ROOT/lib/* "$BOOST_MERGED/lib/" || true

export BOOST_ROOT=$BOOST_MERGED
export BOOST_INCLUDEDIR=$BOOST_MERGED/include
export BOOST_LIBRARYDIR=$BOOST_MERGED/lib

# ==============================
# CLONA O AGGIORNA MEVACOIN
# ==============================
mkdir -p "$WORKDIR"
cd "$WORKDIR"

echo "==> Clonaggio/Aggiornamento repository Mevacoin..."
if [ ! -d "mevacoin_for_android/.git" ]; then
    rm -rf mevacoin_for_android
    git clone "$REPO_URL" mevacoin_for_android
else
    cd mevacoin_for_android
    git reset --hard
    git pull
    cd ..
fi
cd mevacoin_for_android

# ==============================
# COMPILAZIONE ROCKSDB PER ANDROID
# ==============================
echo "==> Compilazione RocksDB per $ABI..."
cd external/rocksdb
rm -rf build-android
mkdir -p build-android out/$ABI/lib out/$ABI/include
cd build-android

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_PATH/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=$ABI \
  -DANDROID_PLATFORM=android-$ANDROID_API \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DWITH_TESTS=OFF \
  -DWITH_TOOLS=OFF \
  -DWITH_BENCHMARK_TOOLS=OFF \
  -DCMAKE_BUILD_TYPE=Release

make -j "$NUM_CORES" rocksdb

# Copia include e libreria statica
cp -r ../include ../out/$ABI/
cp librocksdb.a ../out/$ABI/lib/

cd ../../

# ==============================
# PULIZIA BUILD PRECEDENTE
# ==============================
rm -rf build
mkdir -p build
cd build

# ==============================
# CONFIGURAZIONE CMAKE PER MEVACOIN
# ==============================
echo "==> Configurazione CMake per Android $ABI..."
CMAKE_TOOLCHAIN="$ANDROID_NDK_PATH/build/cmake/android.toolchain.cmake"
if [ ! -f "$CMAKE_TOOLCHAIN" ]; then
    echo "Errore: toolchain file non trovato: $CMAKE_TOOLCHAIN"
    exit 1
fi

ROCKSDB_ROOT=$WORKDIR/mevacoin_for_android/external/rocksdb/out/$ABI
ROCKSDB_INCLUDE=$ROCKSDB_ROOT/include
ROCKSDB_LIB=$ROCKSDB_ROOT/lib/librocksdb.a

cmake .. \
  -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN" \
  -DANDROID_ABI=$ABI \
  -DANDROID_PLATFORM=android-$ANDROID_API \
  -DBOOST_ROOT="$BOOST_ROOT" \
  -DBOOST_INCLUDEDIR="$BOOST_INCLUDEDIR" \
  -DBOOST_LIBRARYDIR="$BOOST_LIBRARYDIR" \
  -DRocksDB_INCLUDE_DIR="$ROCKSDB_INCLUDE" \
  -DRocksDB_LIBRARY="$ROCKSDB_LIB" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_CXX_FLAGS="-I$BOOST_INCLUDEDIR -I$ROCKSDB_INCLUDE -D__ANDROID__ -DANDROID -Wno-error -Wno-shadow -Wno-deprecated-copy" \
  -DFAIL_ON_WARNINGS=OFF

# ==============================
# COMPILAZIONE BINARI
# ==============================
echo "==> Compilazione binari Mevacoin..."
# Salva stdout e stderr in build.log
make -j "$NUM_CORES" > "$WORKDIR/build.log" 2>&1 || {
    echo "==> La compilazione ha generato errori. Controlla $WORKDIR/build.log"
    exit 1
}


# ==============================
# PACCHETTO BINARI
# ==============================
echo "==> Creazione cartella output..."
OUTPUT_DIR="$WORKDIR/output"
mkdir -p "$OUTPUT_DIR"

find src -maxdepth 1 -type f -executable -exec cp {} "$OUTPUT_DIR/" \;

echo "==> Build completata! Binari disponibili in: $OUTPUT_DIR"
ls -l "$OUTPUT_DIR"
