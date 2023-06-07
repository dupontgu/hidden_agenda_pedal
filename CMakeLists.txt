
cmake_minimum_required(VERSION 3.13)
include (pico_sdk_import.cmake)
project(pio_usb)

pico_sdk_init()

# Last checked Pico PIO USB build on main, commit: 9ff3f52f
if (DEFINED ENV{PICO_PIO_USB_DIR} AND (NOT PICO_PIO_USB_DIR))
    set(PICO_PIO_USB_DIR $ENV{PICO_PIO_USB_DIR})
    message("Using PICO_PIO_USB_DIR from environment ('${PICO_PIO_USB_DIR}')")
endif ()

if (NOT PICO_PIO_USB_DIR)
    message(FATAL_ERROR "Please specify the directory for the Pico PIO USB library on your machine using env variable PICO_PIO_USB_DIR")
endif ()
# the second argument to add_subdirectory is needed here because this isn't
# a subdirectory, it's out of tree.
add_subdirectory(${PICO_PIO_USB_DIR} pico_pio_usb)

add_subdirectory(src)

