Per eseguire la compilazione scarica prima Dockerfile.android e poi costruisci l'immagine 

docker build --tag mevacoin-android --build-arg THREADS=4 -f Dockerfile.android .

E

Poi lancia la build

docker run --rm -it \
  -v /opt/mevacoin:/mevacoin \
  -w /mevacoin \
  mevacoin-android \
  bash -c "mkdir -p build-android && cd build-android && cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=\$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-28 \
    -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc)"

