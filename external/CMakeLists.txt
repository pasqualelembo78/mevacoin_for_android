cmake_minimum_required(VERSION 3.10)
project(MevacoinExternal)

# ==============================================================
# Impostazioni miniupnpc
# ==============================================================
set(UPNPC_BUILD_STATIC ON CACHE BOOL "Build static library")
set(UPNPC_BUILD_SHARED OFF CACHE BOOL "Build shared library")
set(UPNPC_BUILD_TESTS OFF CACHE BOOL "Build test executables")
set(WITH_MD_LIBRARY OFF CACHE BOOL "Code Generation for static library")

add_subdirectory(miniupnpc)

# ==============================================================
# Impostazioni CryptoPP
# ==============================================================
add_subdirectory(cryptopp)

# ==============================================================
# RocksDB
# ==============================================================
# Per MSVC si può ancora compilare RocksDB locale
if(MSVC)
    add_subdirectory(rocksDB EXCLUDE_FROM_ALL)
    set_property(TARGET upnpc-static cryptopp-static rocksdb PROPERTY FOLDER "external")
else()
    # Su Android / Linux / Clang usiamo la libreria precompilata già importata nel main CMake
    set_property(TARGET upnpc-static cryptopp-static PROPERTY FOLDER "external")
endif()

# ==============================================================
# Flags di compilazione per warning
# ==============================================================
if(MSVC)
    set_property(TARGET upnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -wd4244 -wd4267")
else()
    set_property(TARGET upnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-undef -Wno-unused-result -Wno-unused-value")
endif()
