#!/bin/bash
set -e
set -x

# =========================
# Variabili principali
# =========================
ANDROID_HOME="$HOME/Android/Sdk"
CMDLINE_TOOLS_VERSION="9123335_latest"  # aggiorna se necessario
NDK_VERSION="25.2.9519653"              # versione NDK desiderata
PLATFORM_TOOLS_VERSION="latest"

# Creazione cartelle
mkdir -p "$ANDROID_HOME/cmdline-tools"
mkdir -p "$ANDROID_HOME/ndk"
mkdir -p "$ANDROID_HOME/platform-tools"

# =========================
# Aggiorna sistema e installa dipendenze
# =========================
sudo apt update
sudo apt install -y unzip curl wget openjdk-17-jdk git

# =========================
# Scarica e installa cmdline-tools
# =========================
cd /tmp
wget https://dl.google.com/android/repository/commandlinetools-linux-${CMDLINE_TOOLS_VERSION}.zip -O cmdline-tools.zip
unzip cmdline-tools.zip -d "$ANDROID_HOME/cmdline-tools"
mv "$ANDROID_HOME/cmdline-tools/cmdline-tools" "$ANDROID_HOME/cmdline-tools/latest"
rm cmdline-tools.zip

# =========================
# Aggiungi variabili d'ambiente
# =========================
grep -qxF "export ANDROID_HOME=$ANDROID_HOME" ~/.bashrc || echo "export ANDROID_HOME=$ANDROID_HOME" >> ~/.bashrc
grep -qxF "export PATH=\$ANDROID_HOME/cmdline-tools/latest/bin:\$ANDROID_HOME/platform-tools:\$PATH" ~/.bashrc || \
    echo "export PATH=\$ANDROID_HOME/cmdline-tools/latest/bin:\$ANDROID_HOME/platform-tools:\$PATH" >> ~/.bashrc
export ANDROID_HOME="$ANDROID_HOME"
export PATH="$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$PATH"

# =========================
# Accetta licenze SDK
# =========================
yes | sdkmanager --licenses

# =========================
# Installa piattaforme e strumenti Android
# =========================
sdkmanager "platform-tools" "platforms;android-33" "build-tools;33.0.2" "ndk;$NDK_VERSION" "cmake;3.22.1"

# =========================
# Mostra configurazione finale
# =========================
echo "========================"
echo "Android SDK installato in: $ANDROID_HOME"
echo "NDK versione: $NDK_VERSION"
echo "Aggiungi al tuo terminale: source ~/.bashrc"
echo "verificare con $ANDROID_NDK altrimenti usa" 
echo "xport ANDROID_NDK=/Android/Sdk/ndk/25.2.9519653" 
echo "========================"
