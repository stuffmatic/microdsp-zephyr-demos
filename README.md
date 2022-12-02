

# Zephyr rust audio demo

Result of research.
Part note to self, part boilerplate. May help someone looking to get into embedded sound.

This is a Zephyr demo app demonstrating

* how to do real time audio processing using I2S and an external audio codec
* how use rust crates in a Zephyr app

The app was built with nRF Connect SDK v2.1.2 and has been tested with the following boards:

* [nRF52840 dev board]()
* [nRF5340 dev board]()


## Audio codecs

Rudimentary drivers for

* WM8758b - min MCLK 8MHz, nRF52840 max MCLK 4 MHz :(
* WM8940

Codecs not requiring drivers, like [BLA]

## Using rust crates in Zephyr

* install target. `Could not find specification for target "thumbv8m.main-none-eabih"`. https://rust-lang.github.io/rustup/cross-compilation.html.  

### Rust lib from C

The basics of using rust crates from C code are covered [here](https://docs.rust-embedded.org/book/interoperability/rust-with-c.html).

### Memory management

* avoid Box via stack. Allocate arrays via vec. Custom global allocator
* statically allocated?


### Adding rust dependencies in Zephyr

[`AddRustLib.cmake`](AddRustLib.cmake) contains the CMake function `add_rust_lib`, which makes it easy to add rust library crate dependencies to a Zephyr app. Each crate dependency is compiled to a static library and the directory containing its C headers is added to the header search paths. Crate dependencies are built automatically as part of the Zephyr build process and debugging should work seamlessly. For example usage, see [CMakeLists.txt](CMakeLists.txt).

TODO: Duplicate symbols issues
