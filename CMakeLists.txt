
cmake_minimum_required(VERSION 3.13)
include (pico_sdk_import.cmake)
project(pio_usb)

pico_sdk_init()

# add pico-pio-usb library for examples
set(PICO_PIO_USB_DIR "${CMAKE_CURRENT_LIST_DIR}/../Pico-PIO-USB")
# the second argument to add_subdirectory is needed here because this isn't
# a subdirectory, it's out of tree.
add_subdirectory(${PICO_PIO_USB_DIR} pico_pio_usb)

add_subdirectory(src)

