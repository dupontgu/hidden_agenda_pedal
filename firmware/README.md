# Hidden Agenda Firmware
* The pedal is built around the [Raspberry Pi RP2040](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html) microcontroller.
* The firmware is written in C/C++, using the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).
* It leverages the [Pico PIO USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) library, which gives the RP2040 an extra (bit-banged) USB port.
* See the [hardware](../hardware/) section to match up pin numbers.

## Building (Linux)
1. Ensure CMAKE is installed.
1. Clone the Pico SDK and all of its submodules. Export the SDK's path to an environment variable called `PICO_SDK_PATH`.
1. Clone the Pico PIO USB library. Export the library's path to an environment variable called `PICO_PIO_USB_DIR`.
1. Clone this repository, `cd` into the `firmware` directory.
1. Create a build directory and use `cmake` to configure the build:
    ```
    $ mkdir build && cd build
    $ cmake -DPICO_BOARD=ha_board ..
    ```
1. Run the build (you can run this step without the previous ones from here on out):
    ```
    $ make
    ```
1. Find the compiled UF2 file at `build/src/hidden_agenda.uf2` - this can loaded onto the RP2040 while it is in USB bootloader mode.

## Running Tests (Any Platform?)
Code in the [`common`](common) directory should be platform agnostic. Therefore it is possible to run tests against this code on your build machine. The `test` directory will contain an executable that runs default tests. To build this executable, follow the steps above, but add the `TEST` flag when running cmake:
```
cmake -DTEST=ON ..
```
If you've previously built the firmware, you'll have to delete the `CMakeCache.txt` file in the build directory. You'll have to delete this each time you change the `TEST` flag.