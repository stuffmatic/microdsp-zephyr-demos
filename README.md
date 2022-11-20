

# Zephyr rust audio demo

This is a Zephyr demo app for the [nRF52840 dev board]() showing

* how to use the I2S peripheral of the nRF52840 to do real time audio processing using an external audio codec
* how use rust crates in a Zephyr app

The app was built and tested with nRF Connect SDK v2.1.2.

## Supported codecs

* Bla
* Bla
* Bla

See [bla]() for dev boards.

## Using rust crates in Zephyr

[`AddRustLib.cmake`](AddRustLib.cmake) contains the CMake function `add_rust_lib`, which makes it easy to add rust library crate dependencies to a Zephyr app. Each crate dependency is compiled to a static library and the directory containing its C headers is added to the header search paths. Crate dependencies are built automatically as part of the Zephyr build process and debugging should work seamlessly. For example usage, see [CMakeLists.txt](CMakeLists.txt). 

The basics of using rust crates from C code are covered [here](https://docs.rust-embedded.org/book/interoperability/rust-with-c.html).