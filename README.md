

# Zephyr rust audio demo

This is a Zephyr demo app for the [nRF52840 dev board]() showing

* how to use the I2S peripheral of the nRF52840 to do real time audio processing using an external audio codec
* how use rust crates in a Zephyr app

The app was built and tested with nRF Connect SDK v2.1.2.


## Using rust crates in Zephyr

A rust library crate can be used from C code by compiling it to a library that is linked into the final binary. The API of the library is specified using a C header file. How this is done is [fairly well documented](https://docs.rust-embedded.org/book/interoperability/rust-with-c.html). It's less obvious how this process can be integrated into a Zephyr project. One solution is to define a reusable CMake function for adding library crate dependencies. This is the strategy used in this demo app and has the following benefits:

* debugging works seamlessly
* adding and removing rust crate dependencies is easy
* crates are built automatically as part of the Zephyr build process
* all CMake magic is contained in AddRustLib.cmake 

Let's assume 

* `AddRustLib.cmake` is in the app root folder next to `CMakeLists.txt`. 
* The app root folder contains a library crate called `my-rust-lib` in a folder called `my_rust_lib`
*  a library crate dependency can be added by appending the following lines to CMakeLists.txt:

```
include(AddRustLib.cmake)
add_rust_lib(
  my-rust-lib
  ${CMAKE_CURRENT_SOURCE_DIR}/my_rust_lib
  ${CMAKE_CURRENT_SOURCE_DIR}/my_rust_lib/include
)
```
