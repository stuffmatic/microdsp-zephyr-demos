# microdsp Zephyr demos

This is a collection of demos showing how to do real time, full duplex audio processing on a microcontroller using [Zephyr](https://zephyrproject.org/) ([nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html])) and the [microdsp](https://github.com/stuffmatic/microdsp) Rust library.

The [microdsp_demos](microdsp_demos) Rust crate contains the demo apps and is added to the Zephyr build using [`zephyr_add_rust_library`](https://github.com/stuffmatic/zephyr_add_rust_library).

The demo apps have been tested with nRF Connect SDK v2.1.2 and the following boards.

* [nRF52840 DK](https://www.nordicsemi.com/Products/Development-hardware/nRF52840-DK)
* [nRF5340 DK](https://www.nordicsemi.com/Products/Development-hardware/nRF5340-DK)

## Demos

In the videos below, an nRF52840 DK board is used together with breakout boards for a [WM8940](datasheets/1912111437_Cirrus-Logic-WM8904CGEFL-RV_C323845.pdf) audio codec and an analog MEMS microphone. KiCad projects and JLCPCB fabrication files for these breakout boards are available [here](https://github.com/stuffmatic/kicad-boards).

### Normalized least mean squares filter

This demo shows how to use a normalized least mean squares (NLMS) filter to reduce leakage of sound from the speaker in the signal recorded by the microphone.

https://user-images.githubusercontent.com/2444852/212777121-3747b8f9-168c-45c7-9448-de3e958bd204.mov

* __Button 1__ - Toggle speaker output
* __Button 2__ - Toggle NLMS filter
* __Button 3__ - Toggle recording
* __Button 4__ - Toggle playback
* __LED 1__ - On when speaker output is active
* __LED 2__ - On when the NLSM filter is active
* __LED 3__ - On when recording
* __LED 4__ - On when playing back recording

### MPM pitch detection demo

A (very) simple ukulele tuner. Each of the four LEDs turns on when a pitch close to the corresponding ukulele string is detected.

https://user-images.githubusercontent.com/2444852/212776229-e64f0f90-aea1-44fe-b742-57fd5b93aa86.mov

* __LED 1__ - On when the pitch is close to 392 Hz
* __LED 2__ - On when the pitch is close to 262 Hz
* __LED 3__ - On when the pitch is close to 330 Hz
* __LED 4__ - On when the pitch is close to 440 Hz

### Spectral flux novelty detection demo

This demo shows how to detect transients and "starts of sounds" using spectral changes over time rather than just signal amplitude changes.

https://user-images.githubusercontent.com/2444852/212776266-3e04822a-fc46-4194-af9e-635a33bfee35.mov

* __LED 1__ - Toggles between on and off for each detected novelty peak

## Selecting which demo to build

The [`microdsp_demos`](microdsp_demos) Rust crate is compiled as part of the Zephyr build using [`zephyr_add_rust_library`](https://github.com/stuffmatic/zephyr_add_rust_library), which is called from [CMakeLists.txt](CMakeLists.txt). The `EXTRA_CARGO_ARGS` argument is used to specify which demo app to build by enabling one of the following cargo features:

* `nlms_demo` - Normalized least mean squares filter demo
* `sfnov_demo` -Spectral flux novelty detection demo
* `mpm_demo` - MPM pitch detection demo
