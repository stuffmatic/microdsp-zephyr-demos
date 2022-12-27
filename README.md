# microdsp Zephyr demos 

This is a [Zephyr](https://zephyrproject.org/) ([nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html])) app showing how to do real time, full duplex audio processing on a microcontroller using the [microdsp](https://github.com/stuffmatic/microdsp) Rust library and an external audio codec. The app has been tested with nRF Connect SDK v2.1.2 and the following boards

* [nRF52840 DK](https://www.nordicsemi.com/Products/Development-hardware/nRF52840-DK)
* [nRF5340 DK](https://www.nordicsemi.com/Products/Development-hardware/nRF5340-DK)

Rudimentary drivers are available for the following audio codecs

* [WM8758b](datasheets/1811051126_Cirrus-Logic-WM8758CBGEFL-RV_C323840.pdf) (not compatible with nRF52840 DK due to clocking limitations)
* [WM8940](datasheets/1912111437_Cirrus-Logic-WM8904CGEFL-RV_C323845.pdf)

The demos in the videos below run on an nRF52840 DK with breakout boards for

* a WM8940 audio codec 
* an analog MEMS microphone
* a power amplifier driving a small speaker

KiCad projects and JLCPCB fabrication files for these breakout boards are available [here](https://github.com/stuffmatic/kicad-boards).


## Demos

### Normalized lol face

### Pitch 

### Pitch 


## Selecting which demo to build

The [`microdsp_demos`](microdsp_demos) folder contains a Rust crate with a number of demo apps exposing C APIs used from the Zephyr app. This crate is compiled as part of the Zephyr build using [`zephyr_add_rust_library`](https://github.com/stuffmatic/zephyr_add_rust_library), see [CMakeLists.txt](CMakeLists.txt). To select which demo to build, enable one of the following cargo features

* `nlms_demo` - Normalized least squares filter demo
* `sfnov_demo` - Novelty detection demo
* `mpm_demo` - MPM pitch detection demo