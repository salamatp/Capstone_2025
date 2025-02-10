cmake_minimum_required(VERSION 3.13) 

set(PICO_SDK_PATH /home/arnur/Capstone_2025/PICO/pico-sdk)
set(KISSFFT_PATH /home/arnur/Capstone_2025/PICO/Libraries/kissfft)

# include(pico_sdk_import.cmake)
include_directories(chordDetect ${KISSFFT_PATH}/)

add_library(kissfft STATIC 
    ${KISSFFT_PATH}/kiss_fft.c
    ${KISSFFT_PATH}/kiss_fftr.c
    ${KISSFFT_PATH}/kiss_fft.h
    ${KISSFFT_PATH}/kiss_fftr.h
)
set_target_properties(kissfft PROPERTIES LINKER_LANGUAGE C)
target_include_directories(kissfft PUBLIC ${KISSFFT_PATH})

set(KISSFFT_SRC
    ${KISSFFT_PATH}/kiss_fftr.c
)

project(capstone_project C CXX ASM) # CXX is for C++
set(CMAKE_CXX_STANDARD 17) # We are using C++

# pico_sdk_init() # Initialize the Pico SDK within the project 

# ===========================================
# Build for Raspberry Pi Pico
# ===========================================
add_executable(chordDetect chordDetect.cpp)

# pico_enable_stdio_usb(chordDetect 1) # Enable USB output
# pico_enable_stdio_uart(chordDetect 1) # Enable UART output
# pico_add_extra_outputs(chordDetect) 

target_link_libraries(chordDetect  kissfft)
message("GOODBYE!") 
# ===========================================
# Extra: Build for Linux (Optional Debugging)
# # ===========================================
# if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
message("Howdy! You are building for Linux!")    
add_executable(chordDetect_linux chordDetect.cpp)
target_compile_definitions(chordDetect_linux PRIVATE PICO_ON_LINUX=1)
target_link_libraries(chordDetect_linux kissfft m) # Link math library if needed
# endif()