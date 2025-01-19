cmake_minimum_required(VERSION 3.13) 
set(PICO_SDK_PATH "/home/arnur/Capstone_2025/PICO/pico-sdk")


include(pico_sdk_import.cmake)
project(capstone_project C CXX ASM) # CXX is for C++
set(CMAKE_CXX_STANDARD 17) # set(CMAKE_C_STANDARD 11) We are not using C

pico_sdk_init() # Initialize the Pico SDK within the project 
add_executable(chordDetect chordDetect.cpp) # Add the source file to the project and name the executable chordDetect

pico_enable_stdio_usb(chordDetect 1) # Enable USB output
pico_enable_stdio_uart(chordDetect 1) # Enable UART output
pico_add_extra_outputs(chordDetect) 

target_link_libraries(chordDetect pico_stdlib hardware_spi)
