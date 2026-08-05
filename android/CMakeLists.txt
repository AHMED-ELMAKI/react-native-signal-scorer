project(NitroSignalScorer)
cmake_minimum_required(VERSION 3.9.0)

set (PACKAGE_NAME NitroSignalScorer)
set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 20)

# Define C++ library and add all sources
add_library(${PACKAGE_NAME} SHARED
        ../cpp/HybridTickScorer.cpp
        ../cpp/HybridSignalTickerFactory.cpp
        ../cpp/rust/TickerRustBridge.cpp
)

# Add Nitrogen specs :)
include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/NitroSignalScorer+autolinking.cmake)

# Resolve the Rust static archive for the current build ABI.
# The Rust core is built from source via cargo-ndk (see build.gradle) and
# emitted into the Cargo target directory keyed by the Rust target triple.
set(RUST_LIB_PATH "")
set(RUST_TARGET "")
if(ANDROID_ABI STREQUAL "arm64-v8a")
  set(RUST_TARGET "aarch64-linux-android")
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
  set(RUST_TARGET "armv7-linux-androideabi")
elseif(ANDROID_ABI STREQUAL "x86")
  set(RUST_TARGET "i686-linux-android")
elseif(ANDROID_ABI STREQUAL "x86_64")
  set(RUST_TARGET "x86_64-linux-android")
endif()

if(NOT RUST_TARGET STREQUAL "")
  set(RUST_LIB_PATH "${CMAKE_SOURCE_DIR}/../rust/target/${RUST_TARGET}/release/libsignal_ticker_rust.a")
endif()

# Set up local includes
include_directories(
        "../cpp"
        "../nitrogen/generated/shared/c++"
)

find_library(LOG_LIB log)

target_link_libraries(
        ${PACKAGE_NAME}
        ${LOG_LIB}
        android
)

if(NOT RUST_LIB_PATH STREQUAL "" AND EXISTS "${RUST_LIB_PATH}")
  target_link_libraries(${PACKAGE_NAME} "${RUST_LIB_PATH}")
else()
  message(WARNING "NitroSignalScorer: Rust static library not found at ${RUST_LIB_PATH}; run cargo-ndk build first (see build.gradle compileSignalTickerRust task)")
endif()
