
cmake_minimum_required(VERSION 3.13)
option(TEST "Building test executable" OFF)

if(TEST)
    project(hidden_agenda_test)
    add_subdirectory(test)
else()
    execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(PICO_BOARD_HEADER_DIRS "${CMAKE_SOURCE_DIR}/src") 
    include (pico_sdk_import.cmake)
    project(hidden_agenda_project)
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
endif(TEST)

add_subdirectory(common)



