Procedura 

1. Installare ambiente Android completo
2. Scaricare mevacoin in /opt/mevacoin
3. Compilare boost per Android con github.com
4. Per eseguire la compilazione scarica prima Dockerfile.android e poi costruisci l'immagine 

docker build --tag mevacoin-android --build-arg THREADS=4 -f Dockerfile.android .

E

Poi lancia la build

docker run --rm -it \
  -v /opt/mevacoin:/mevacoin \
  -v /opt/boost:/opt/boost \
  -w /mevacoin \
  mevacoin-android \
  bash -c "export BOOST_ROOT=/opt/boost/build/out/arm64-v8a && \
           mkdir -p build-android && cd build-android && \
           cmake .. \
           -DCMAKE_TOOLCHAIN_FILE=\$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
           -DANDROID_ABI=arm64-v8a \
           -DANDROID_PLATFORM=android-28 \
           -DCMAKE_BUILD_TYPE=Release \
           -DBoost_USE_STATIC_LIBS=ON \
           -DBoost_NO_SYSTEM_PATHS=ON && \
           make -j\$(nproc)"


