# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rust_audio_demo)

target_sources(app PRIVATE src/main.c)

# Rust lib
# https://discourse.cmake.org/t/problems-when-porting-a-library-target-from-externalproject-add/3522

add_library(rust_lib INTERFACE librust_lib.a)
target_include_directories(rust_lib INTERFACE /Users/perarne/code/zephyr/zerphyr-rust-audio-demo/rust_lib/include)
target_link_libraries(rust_lib INTERFACE /Users/perarne/code/zephyr/zerphyr-rust-audio-demo/rust_lib/target/thumbv7em-none-eabihf/debug/librust_lib.a)
target_link_libraries(app PRIVATE rust_lib)

add_custom_command(
  OUTPUT librust_lib.a
  COMMAND cargo build --target thumbv7em-none-eabihf
  WORKING_DIRECTORY ../rust_lib
  COMMENT "Building rust_lib"
  VERBATIM
)


