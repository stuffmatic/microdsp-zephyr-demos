# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rust_audio_demo)

target_sources(app PRIVATE src/main.c src/i2s.c src/leds.c src/buttons.c src/codecs/wm8758.c src/codecs/wm8904.c)

# Import the zephyr_add_rust_library function
include(zephyr_add_rust_library.cmake)

# Add microdsp-demo crate dependency.
zephyr_add_rust_library(
  CRATE_NAME microdsp-demo # Crate name as specified in Cargo.toml
  CRATE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/microdsp_demo # Crate root dir
  CRATE_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/microdsp_demo/include # C header dir
  CARGO_PROFILE release
  EXTRA_CARGO_ARGS --no-default-features --features nlms_demo # Select demo to build
)