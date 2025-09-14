Procedura 

1. Installare ambiente Android completo
2. Scaricare mevacoin in /opt/mevacoin
3. Compilare boost per Android con github.com
4. Per eseguire la compilazione scarica prima Dockerfile.android e poi costruisci l'immagine 

docker build --tag mevacoin-android --build-arg THREADS=4 -f Dockerfile.android .

E

Poi lancia la build

docker run --rm -it -v /opt/mevacoin:/mevacoin -v /opt/boost/build/out/arm64-v8a/include:/opt/boost/include -v /opt/boost/build/out/arm64-v8a/lib:/opt/boost/lib -w /mevacoin mevacoin-android bash -c "export BOOST_ROOT=/opt/boost && mkdir -p build-android && cd build-android && cmake .. -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)"


Se non riconosce boost
Aggiorna i simlink

cd /opt/boost/build/out/arm64-v8a/lib

ln -s libboost_system-clang-mt-a64-1_85.a libboost_system.a
ln -s libboost_filesystem-clang-mt-a64-1_85.a libboost_filesystem.a
ln -s libboost_thread-clang-mt-a64-1_85.a libboost_thread.a
ln -s libboost_date_time-clang-mt-a64-1_85.a libboost_date_time.a
ln -s libboost_chrono-clang-mt-a64-1_85.a libboost_chrono.a
ln -s libboost_regex-clang-mt-a64-1_85.a libboost_regex.a
ln -s libboost_serialization-clang-mt-a64-1_85.a libboost_serialization.a
ln -s libboost_program_options-clang-mt-a64-1_85.a libboost_program_options.a



mkdir -p /opt/boost/include
ln -s /opt/boost/build/out/arm64-v8a/include/boost-1_85 /opt/boost/include/boost-1_85

Se non riconosce o non trova le librerie 

cd /opt/mevacoin/rocksdb


cd build-android
rm -rf *

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_TESTS=OFF \
  -DWITH_TOOLS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON

Alla fine della compilazione puoi ridurre le dimensioni del file rimuovendo i simboli.
I file compilati si troveranno in /opt/mevacoin/build-android/src

Fai così :

Controlla innanzitutto su echo $ANDROID_NDK porta al ndk, altrimenti fai echo 'export ANDROID_NDK=/root/Android/Sdk/ndk/25.2.9519653' >> ~/.bashrc
poi controlliamo anche $ANDROID_NDK_ROOT  se è corretto, altrimenti fai echo 'export ANDROID_NDK_ROOT=/root/Android/Sdk/ndk/25.2.9519653' >> ~/.bashrc
source ~/.bashrc


Per renderla permanentr

Se hai fatto bene tutto dovrebbero stare tutto in Android. 

Poi strippare il contenuto di src compilato con $ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip mevacoind miner mevacoin-service wallet-api xkrwallet xkrwallet-beta crypto_test

Fatto. Così hai i binari Eseguibili 

Con questo hai i so

docker run --rm -it \
-v /opt/mevacoin:/mevacoin \
-v /opt/boost/build/out/arm64-v8a/include:/opt/boost/include \
-v /opt/boost/build/out/arm64-v8a/lib:/opt/boost/lib \
-w /mevacoin \
mevacoin-android bash -c "\
export BOOST_ROOT=/opt/boost && \
mkdir -p build-android-so && cd build-android-so && \
cmake .. \
-DCMAKE_TOOLCHAIN_FILE=/opt/android-sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake \
-DANDROID_ABI=arm64-v8a \
-DANDROID_PLATFORM=android-28 \
-DCMAKE_BUILD_TYPE=Release \
-DBUILD_SHARED_LIBS=ON \
-G Ninja && \
ninja -j$(nproc)"














