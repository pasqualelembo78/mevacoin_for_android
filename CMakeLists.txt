# ==============================================================
# Mevacoin CMakeLists.txt per Android
# ==============================================================
cmake_minimum_required(VERSION 3.10)

project(Mevacoin)

# Attivazione ccache se presente
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache package... Activating...")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Abilita linguaggi
enable_language(C)
enable_language(CXX)

# Testnet
option(TEST_NET "Compile a testnet version" OFF)
if(TEST_NET)
    message(STATUS "TEST_NET: ON")
    add_definitions(-DUSE_TESTNET)
endif()

# Controllo compilatore
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
        message(FATAL_ERROR "GCC/G++ 7.0 o maggiore richiesto")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
        message(FATAL_ERROR "Clang 6.0 o maggiore richiesto")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.10)
        message(FATAL_ERROR "MSVC 19.10 o maggiore richiesto")
    endif()
else()
    message(WARNING "Compilatore non supportato")
endif()

find_package(Threads)

# ==============================================================
# Configurazioni generali
# ==============================================================
set(VERSION "0.1")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo Release CACHE TYPE INTERNAL)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
endif()
# ==============================================================
# Include cpu-features per Android NDK
# ==============================================================
# Usa i file direttamente dall'NDK
set(CPUFEATURES_DIR $ENV{ANDROID_NDK_ROOT}/sources/android/cpufeatures)

include_directories(${CPUFEATURES_DIR})

add_library(cpufeatures STATIC ${CPUFEATURES_DIR}/cpu-features.c)
# ==============================================================
# RocksDB precompilato
# ==============================================================
add_library(rocksdb STATIC IMPORTED)
set_target_properties(rocksdb PROPERTIES
    IMPORTED_LOCATION /mevacoin/external/rocksdb/build-android/librocksdb.a
    INTERFACE_INCLUDE_DIRECTORIES /mevacoin/external/rocksdb/include
)

# ==============================================================
# Opzioni Slow Hash / ARM
# ==============================================================
set(FORCE_USE_HEAP ON CACHE BOOL "Force heap allocation")
set(NO_AES OFF CACHE BOOL "Turn off AES")
set(NO_OPTIMIZED_MULTIPLY_ON_ARM OFF CACHE BOOL "Turn off optimized multiply on ARM")

if(FORCE_USE_HEAP)
    add_definitions(-DFORCE_USE_HEAP)
    message(STATUS "FORCE_USE_HEAP: ENABLED")
endif()
if(NO_AES)
    add_definitions(-DNO_AES)
    message(STATUS "HW AES: DISABLED")
endif()
if(NO_OPTIMIZED_MULTIPLY_ON_ARM)
    add_definitions(-DNO_OPTIMIZED_MULTIPLY_ON_ARM)
    message(STATUS "OPTIMIZED_ARM_MULTIPLICATION: DISABLED")
endif()

# ==============================================================
# Controllo architettura
# ==============================================================
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Targeting 32-bit non supportato")
endif()

# ==============================================================
# Git versioning
# ==============================================================
set(COMMIT_ID_IN_VERSION ON CACHE BOOL "Include commit ID in version")
add_definitions(-DRAPIDJSON_HAS_STDSTRING)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/version")

if(NOT COMMIT_ID_IN_VERSION)
    set(VERSION "${VERSION}-unknown")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version/version.h")
    add_custom_target(version ALL)
else()
    find_package(Git QUIET)
    if(Git_FOUND OR GIT_FOUND)
        add_custom_target(version ALL
            "${CMAKE_COMMAND}" -DVERSION=${VERSION} -DGIT=${GIT_EXECUTABLE}
            -DTO=${CMAKE_CURRENT_BINARY_DIR}/version/version.h
            -P "${CMAKE_CURRENT_SOURCE_DIR}/src/version.cmake"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    else()
        message(STATUS "WARNING: Git non trovato!")
        set(VERSION "${VERSION}-unknown")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version/version.h")
        add_custom_target(version ALL)
    endif()
endif()

# ==============================================================
# Include directories principali
# ==============================================================
include_directories(
    include
    "${CMAKE_CURRENT_BINARY_DIR}/version"
    src
    external
)

# ==============================================================
# Platform-specific
# ==============================================================
if(MSVC)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows)
elseif(APPLE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/osx)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/posix)
else()
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/posix)
endif()

# # ==============================================================
# BOOST CONFIG
# ==============================================================
if(NOT DEFINED ENV{BOOST_ROOT})
    message(FATAL_ERROR "BOOST_ROOT environment variable non definita")
endif()

set(BOOST_ROOT "$ENV{BOOST_ROOT}")
# Include diretto alla versione 1.85 se presente
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include/boost-1_85")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")

# Controllo esistenza
if(NOT EXISTS "${BOOST_INCLUDEDIR}")
    message(FATAL_ERROR "Boost include directory non trovato: ${BOOST_INCLUDEDIR}")
endif()
if(NOT EXISTS "${BOOST_LIBRARYDIR}")
    message(FATAL_ERROR "Boost library directory non trovato: ${BOOST_LIBRARYDIR}")
endif()

include_directories(SYSTEM ${BOOST_INCLUDEDIR})
link_directories(${BOOST_LIBRARYDIR})

set(BOOST_LIBS
    boost_system
    boost_filesystem
    boost_thread
    boost_date_time
    boost_chrono
    boost_regex
    boost_serialization
    boost_program_options
)
set(Boost_LIBRARIES ${BOOST_LIBS} CACHE INTERNAL "List of Boost libs to link")

# ==============================================================
# Subdirectories
# ==============================================================
add_subdirectory(external)
add_subdirectory(src)
