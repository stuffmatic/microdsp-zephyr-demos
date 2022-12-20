* support 5340 and 52840 boards
* i2c config arrays
* kconfig settings
* wasm demo
* demo:
  * pmic
  * nrf dk
  * mic
  * codec
  * amp
  * volume pot

# rust build

* change rust code in up to date build. does it work?
* cbindgen
* pass crate type https://github.com/rust-lang/rfcs/pull/3180
* extra cargo arguments (for feature selection etc)
* disassembly
* https://github.com/rust-lang/cargo/issues/1109
* https://www.jaredwolff.com/embedding-rust-into-zephyr-using-cbindgen/
* https://www.youtube.com/watch?v=mfAoYbqUZc4
* use same toolchain as zephyr?
* build into build dir
* multiple definitions error disabled not caught
* test multiple crate deps
* how to detect FPU?