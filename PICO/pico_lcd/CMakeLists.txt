cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH /home/hoangann107/pico/Capstone_2025/PICO/pico-sdk)
#set(ST7789_PATH /home/hoangann107/pico/Capstone_2025/PICO/Libraries/st7789-library-for-pico)

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(my_project)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

#add_subdirectory(st7789-library-for-pico)

# Add executable
add_executable(hello_world 
    hello_world.cpp
)
message("GOODBYE!!!!!!!!!!!!")

# Link ST7789 library and Pico SDK libraries
target_link_libraries(hello_world pico_stdlib hardware_spi)

# Generate binary files
pico_add_extra_outputs(hello_world)

