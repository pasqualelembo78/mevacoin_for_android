# Install script for directory: /mevacoin/external/cryptopp

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/root/Android/Sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/mevacoin/build-android-so/external/cryptopp/libcryptopp.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cryptopp" TYPE FILE FILES
    "/mevacoin/external/cryptopp/3way.h"
    "/mevacoin/external/cryptopp/adler32.h"
    "/mevacoin/external/cryptopp/adv-simd.h"
    "/mevacoin/external/cryptopp/aes.h"
    "/mevacoin/external/cryptopp/algebra.h"
    "/mevacoin/external/cryptopp/algparam.h"
    "/mevacoin/external/cryptopp/arc4.h"
    "/mevacoin/external/cryptopp/argnames.h"
    "/mevacoin/external/cryptopp/aria.h"
    "/mevacoin/external/cryptopp/asn.h"
    "/mevacoin/external/cryptopp/authenc.h"
    "/mevacoin/external/cryptopp/base32.h"
    "/mevacoin/external/cryptopp/base64.h"
    "/mevacoin/external/cryptopp/basecode.h"
    "/mevacoin/external/cryptopp/blake2.h"
    "/mevacoin/external/cryptopp/blowfish.h"
    "/mevacoin/external/cryptopp/blumshub.h"
    "/mevacoin/external/cryptopp/camellia.h"
    "/mevacoin/external/cryptopp/cast.h"
    "/mevacoin/external/cryptopp/cbcmac.h"
    "/mevacoin/external/cryptopp/ccm.h"
    "/mevacoin/external/cryptopp/chacha.h"
    "/mevacoin/external/cryptopp/channels.h"
    "/mevacoin/external/cryptopp/cmac.h"
    "/mevacoin/external/cryptopp/config.h"
    "/mevacoin/external/cryptopp/cpu.h"
    "/mevacoin/external/cryptopp/crc.h"
    "/mevacoin/external/cryptopp/cryptlib.h"
    "/mevacoin/external/cryptopp/default.h"
    "/mevacoin/external/cryptopp/des.h"
    "/mevacoin/external/cryptopp/dh.h"
    "/mevacoin/external/cryptopp/dh2.h"
    "/mevacoin/external/cryptopp/dll.h"
    "/mevacoin/external/cryptopp/dmac.h"
    "/mevacoin/external/cryptopp/drbg.h"
    "/mevacoin/external/cryptopp/dsa.h"
    "/mevacoin/external/cryptopp/eax.h"
    "/mevacoin/external/cryptopp/ec2n.h"
    "/mevacoin/external/cryptopp/eccrypto.h"
    "/mevacoin/external/cryptopp/ecp.h"
    "/mevacoin/external/cryptopp/ecpoint.h"
    "/mevacoin/external/cryptopp/elgamal.h"
    "/mevacoin/external/cryptopp/emsa2.h"
    "/mevacoin/external/cryptopp/eprecomp.h"
    "/mevacoin/external/cryptopp/esign.h"
    "/mevacoin/external/cryptopp/factory.h"
    "/mevacoin/external/cryptopp/fhmqv.h"
    "/mevacoin/external/cryptopp/files.h"
    "/mevacoin/external/cryptopp/filters.h"
    "/mevacoin/external/cryptopp/fips140.h"
    "/mevacoin/external/cryptopp/fltrimpl.h"
    "/mevacoin/external/cryptopp/gcm.h"
    "/mevacoin/external/cryptopp/gf256.h"
    "/mevacoin/external/cryptopp/gf2_32.h"
    "/mevacoin/external/cryptopp/gf2n.h"
    "/mevacoin/external/cryptopp/gfpcrypt.h"
    "/mevacoin/external/cryptopp/gost.h"
    "/mevacoin/external/cryptopp/gzip.h"
    "/mevacoin/external/cryptopp/hashfwd.h"
    "/mevacoin/external/cryptopp/hex.h"
    "/mevacoin/external/cryptopp/hkdf.h"
    "/mevacoin/external/cryptopp/hmac.h"
    "/mevacoin/external/cryptopp/hmqv.h"
    "/mevacoin/external/cryptopp/hrtimer.h"
    "/mevacoin/external/cryptopp/ida.h"
    "/mevacoin/external/cryptopp/idea.h"
    "/mevacoin/external/cryptopp/integer.h"
    "/mevacoin/external/cryptopp/iterhash.h"
    "/mevacoin/external/cryptopp/kalyna.h"
    "/mevacoin/external/cryptopp/keccak.h"
    "/mevacoin/external/cryptopp/lubyrack.h"
    "/mevacoin/external/cryptopp/luc.h"
    "/mevacoin/external/cryptopp/mars.h"
    "/mevacoin/external/cryptopp/md2.h"
    "/mevacoin/external/cryptopp/md4.h"
    "/mevacoin/external/cryptopp/md5.h"
    "/mevacoin/external/cryptopp/mdc.h"
    "/mevacoin/external/cryptopp/mersenne.h"
    "/mevacoin/external/cryptopp/misc.h"
    "/mevacoin/external/cryptopp/modarith.h"
    "/mevacoin/external/cryptopp/modes.h"
    "/mevacoin/external/cryptopp/modexppc.h"
    "/mevacoin/external/cryptopp/mqueue.h"
    "/mevacoin/external/cryptopp/mqv.h"
    "/mevacoin/external/cryptopp/naclite.h"
    "/mevacoin/external/cryptopp/nbtheory.h"
    "/mevacoin/external/cryptopp/network.h"
    "/mevacoin/external/cryptopp/nr.h"
    "/mevacoin/external/cryptopp/oaep.h"
    "/mevacoin/external/cryptopp/oids.h"
    "/mevacoin/external/cryptopp/osrng.h"
    "/mevacoin/external/cryptopp/ossig.h"
    "/mevacoin/external/cryptopp/padlkrng.h"
    "/mevacoin/external/cryptopp/panama.h"
    "/mevacoin/external/cryptopp/pch.h"
    "/mevacoin/external/cryptopp/pkcspad.h"
    "/mevacoin/external/cryptopp/poly1305.h"
    "/mevacoin/external/cryptopp/polynomi.h"
    "/mevacoin/external/cryptopp/ppc-simd.h"
    "/mevacoin/external/cryptopp/pssr.h"
    "/mevacoin/external/cryptopp/pubkey.h"
    "/mevacoin/external/cryptopp/pwdbased.h"
    "/mevacoin/external/cryptopp/queue.h"
    "/mevacoin/external/cryptopp/rabin.h"
    "/mevacoin/external/cryptopp/randpool.h"
    "/mevacoin/external/cryptopp/rc2.h"
    "/mevacoin/external/cryptopp/rc5.h"
    "/mevacoin/external/cryptopp/rc6.h"
    "/mevacoin/external/cryptopp/rdrand.h"
    "/mevacoin/external/cryptopp/resource.h"
    "/mevacoin/external/cryptopp/rijndael.h"
    "/mevacoin/external/cryptopp/ripemd.h"
    "/mevacoin/external/cryptopp/rng.h"
    "/mevacoin/external/cryptopp/rsa.h"
    "/mevacoin/external/cryptopp/rw.h"
    "/mevacoin/external/cryptopp/safer.h"
    "/mevacoin/external/cryptopp/salsa.h"
    "/mevacoin/external/cryptopp/scrypt.h"
    "/mevacoin/external/cryptopp/seal.h"
    "/mevacoin/external/cryptopp/secblock.h"
    "/mevacoin/external/cryptopp/seckey.h"
    "/mevacoin/external/cryptopp/seed.h"
    "/mevacoin/external/cryptopp/serpent.h"
    "/mevacoin/external/cryptopp/serpentp.h"
    "/mevacoin/external/cryptopp/sha.h"
    "/mevacoin/external/cryptopp/sha3.h"
    "/mevacoin/external/cryptopp/shacal2.h"
    "/mevacoin/external/cryptopp/shark.h"
    "/mevacoin/external/cryptopp/simon.h"
    "/mevacoin/external/cryptopp/simple.h"
    "/mevacoin/external/cryptopp/siphash.h"
    "/mevacoin/external/cryptopp/skipjack.h"
    "/mevacoin/external/cryptopp/sm3.h"
    "/mevacoin/external/cryptopp/sm4.h"
    "/mevacoin/external/cryptopp/smartptr.h"
    "/mevacoin/external/cryptopp/socketft.h"
    "/mevacoin/external/cryptopp/sosemanuk.h"
    "/mevacoin/external/cryptopp/speck.h"
    "/mevacoin/external/cryptopp/square.h"
    "/mevacoin/external/cryptopp/stdcpp.h"
    "/mevacoin/external/cryptopp/strciphr.h"
    "/mevacoin/external/cryptopp/tea.h"
    "/mevacoin/external/cryptopp/threefish.h"
    "/mevacoin/external/cryptopp/tiger.h"
    "/mevacoin/external/cryptopp/trap.h"
    "/mevacoin/external/cryptopp/trdlocal.h"
    "/mevacoin/external/cryptopp/trunhash.h"
    "/mevacoin/external/cryptopp/ttmac.h"
    "/mevacoin/external/cryptopp/tweetnacl.h"
    "/mevacoin/external/cryptopp/twofish.h"
    "/mevacoin/external/cryptopp/vmac.h"
    "/mevacoin/external/cryptopp/wait.h"
    "/mevacoin/external/cryptopp/wake.h"
    "/mevacoin/external/cryptopp/whrlpool.h"
    "/mevacoin/external/cryptopp/winpipes.h"
    "/mevacoin/external/cryptopp/words.h"
    "/mevacoin/external/cryptopp/xtr.h"
    "/mevacoin/external/cryptopp/xtrcrypt.h"
    "/mevacoin/external/cryptopp/zdeflate.h"
    "/mevacoin/external/cryptopp/zinflate.h"
    "/mevacoin/external/cryptopp/zlib.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES
    "/mevacoin/external/cryptopp/cryptopp-config.cmake"
    "/mevacoin/build-android-so/external/cryptopp/cryptopp-config-version.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp/cryptopp-targets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp/cryptopp-targets.cmake"
         "/mevacoin/build-android-so/external/cryptopp/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp/cryptopp-targets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp/cryptopp-targets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES "/mevacoin/build-android-so/external/cryptopp/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES "/mevacoin/build-android-so/external/cryptopp/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets-release.cmake")
  endif()
endif()

