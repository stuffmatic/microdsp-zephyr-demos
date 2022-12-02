# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rust_audio_demo)

target_sources(app PRIVATE src/main.c src/i2s.c src/codecs/wm8758.c src/codecs/wm8904.c)

# Import the add_rust_library function
include(AddRustLibrary.cmake)
# Add rust-lib crate dependency.
# ${CMAKE_CURRENT_SOURCE_DIR} is the directory containing CMakeLists.txt.
add_rust_library(
  rust-looper # Crate name as specified in Cargo.toml
  ${CMAKE_CURRENT_SOURCE_DIR}/rust_looper # Crate root dir
  ${CMAKE_CURRENT_SOURCE_DIR}/rust_looper/include # C header dir
)