Procedura 

1. Installare ambiente Android completo
2. Controlla le variabili con

  echo $ANDROID_HOME
echo $ANDROID_NDK_ROOT
echo $PATH | tr ':' '\n' | grep ndk

Se non porta a niente allora definisci con export ANDROID_NDK_ROOT=/root/Android/Sdk/ndk/25.2.9519653

export ANDROID_HOME=/root/Android/Sdk
export ANDROID_SDK_ROOT=/root/Android/Sdk
export ANDROID_NDK_ROOT=/root/Android/Sdk/ndk/25.2.9519653
export PATH=$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/platform-tools:$PATH

oppure in maniera permanente (se funziona correttamente):

echo 'export ANDROID_HOME=/root/Android/Sdk
export ANDROID_SDK_ROOT=/root/Android/Sdk
export ANDROID_NDK_ROOT=/root/Android/Sdk/ndk/25.2.9519653
export PATH=$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/platform-tools:$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH' >> ~/.bashrc && source ~/.bashrc


4. Scaricare mevacoin in /opt/mevacoin
5. Compilare boost per Android con github.com
6. Per eseguire la compilazione scarica prima Dockerfile.android e poi costruisci l'immagine 

docker build --tag mevacoin-android --build-arg THREADS=4 -f Dockerfile.android .

E

Poi lancia la build. In questa maniera compiliamo sia i file statici che dinamici in due cartelle separate. 

docker run --rm -it -v /opt/mevacoin:/mevacoin -v /opt/boost/build/out/arm64-v8a/include:/opt/boost/include -v /opt/boost/build/out/arm64-v8a/lib:/opt/boost/lib -w /mevacoin mevacoin-android bash -c "export BOOST_ROOT=/opt/boost && mkdir -p build-android-so && cd build-android-so && cmake .. -DCMAKE_TOOLCHAIN_FILE=/root/Android/Sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -G Ninja && ninja -j\$(nproc) && cd .. && mkdir -p build-android-a && cd build-android-a && cmake .. -DCMAKE_TOOLCHAIN_FILE=/root/Android/Sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -G Ninja && ninja -j\$(nproc)"


oppure se vuoi compilare sia librerie dinamiche, che librerie statiche e aggiungere anche binari eseguibili

docker run --rm -it -v /opt/mevacoin:/mevacoin -v /opt/boost/build/out/arm64-v8a/include:/opt/boost/include -v /opt/boost/build/out/arm64-v8a/lib:/opt/boost/lib -w /mevacoin mevacoin-android bash -c "export BOOST_ROOT=/opt/boost && mkdir -p build-android-so && cd build-android-so && cmake .. -DCMAKE_TOOLCHAIN_FILE=/root/Android/Sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -G Ninja && ninja -j\$(nproc) && cd .. && mkdir -p build-android-a && cd build-android-a && cmake .. -DCMAKE_TOOLCHAIN_FILE=/root/Android/Sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -G Ninja && ninja -j\$(nproc) && cd .. && mkdir -p build-android-bin && cd build-android-bin && cmake .. -DCMAKE_TOOLCHAIN_FILE=/root/Android/Sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DCMAKE_BUILD_TYPE=Release -DENABLE_BINARIES=ON -G Ninja && ninja -j\$(nproc)"



Se non riconosce boost
Aggiorna i simlink

cd /opt/boost/build/out/arm64-v8a/lib

ln -s libboost_system-clang-mt-a64-1_85.a libboost_system.a && ln -s libboost_filesystem-clang-mt-a64-1_85.a libboost_filesystem.a && ln -s libboost_thread-clang-mt-a64-1_85.a libboost_thread.a && ln -s libboost_date_time-clang-mt-a64-1_85.a libboost_date_time.a && ln -s libboost_chrono-clang-mt-a64-1_85.a libboost_chrono.a && ln -s libboost_regex-clang-mt-a64-1_85.a libboost_regex.a && ln -s libboost_serialization-clang-mt-a64-1_85.a libboost_serialization.a && ln -s libboost_program_options-clang-mt-a64-1_85.a libboost_program_options.a



mkdir -p /opt/boost/include && ln -s /opt/boost/build/out/arm64-v8a/include/boost-1_85 /opt/boost/include/boost-1_85


Se non riconosce o non trova le librerie 

cd /opt/mevacoin/external/rocksdb

Aggiorna cmake e ninja

apt update && apt install -y cmake build-essential ninja-build

Avvia il bash ./build-android.sh 

Alla fine della compilazione puoi ridurre le dimensioni del file rimuovendo i simboli.
I file compilati si troveranno in /opt/mevacoin/build-android/src

Fai così :

Controlla innanzitutto su echo $ANDROID_NDK porta al ndk, altrimenti fai echo 'export ANDROID_NDK=/root/Android/Sdk/ndk/25.2.9519653' >> ~/.bashrc
poi controlliamo anche $ANDROID_NDK_ROOT  se è corretto, altrimenti fai echo 'export ANDROID_NDK_ROOT=/root/Android/Sdk/ndk/25.2.9519653' >> ~/.bashrc
source ~/.bashrc


Per renderla permanente

Se hai fatto bene tutto dovrebbero stare tutto in Android. 

Poi strippare il contenuto di src compilato con 
ROOT=/opt/mevacoin

# Strippa tutti i .so e .a
find "$ROOT" \( -name "*.so" -o -name "*.a" \) -type f -exec /root/Android/Sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip --strip-unneeded {} \;

# Strippa tutti i binari PIE ELF Android
find "$ROOT" -type f -exec sh -c '
  for f do
    if file "$f" | grep -q "ELF" && ! file "$f" | grep -q "shared object"; then
      /root/Android/Sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip --strip-debug "$f"
    fi
  done
' sh {} +


Fatto. 
