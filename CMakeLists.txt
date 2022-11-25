# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rust_audio_demo)

target_sources(app PRIVATE src/main.c src/i2s_nrfx.c src/codec.c)

# Import the add_rust_lib function
include(AddRustLib.cmake)
# Add rust-lib crate dependency.
# ${CMAKE_CURRENT_SOURCE_DIR} is the directory containing CMakeLists.txt.
add_rust_lib(
  rust-lib # Crate name as specified in Cargo.toml
  ${CMAKE_CURRENT_SOURCE_DIR}/rust_lib # Crate root dir
  ${CMAKE_CURRENT_SOURCE_DIR}/rust_lib/include # C header dir
)