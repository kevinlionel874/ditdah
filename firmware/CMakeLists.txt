cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME firmware)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/ditdah/log.c
    Core/Src/ditdah/font5x7.c
    Core/Src/ditdah/sh1106.c
    Core/Src/ditdah/mprls.c
    Core/Src/ditdah/bringup.c
    Core/Src/ditdah/keyboard.c
    Core/Src/ditdah/keyboard_map.c
    Core/Src/ditdah/usb_reconnect.c
    Core/Src/ditdah/inputs.c
    Core/Src/ditdah/feedback.c
    Core/Src/ditdah/display.c
    Core/Src/ditdah/breath_logic.c
    Core/Src/ditdah/breath.c
    Core/Src/ditdah/app.c
    ../decoder/c/morse_decoder.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ../decoder/c
)

# Build the hardware bring-up test (bringup.c) instead of the DitDah app: .\build.ps1 -Bringup
option(DITDAH_BRINGUP "Build the hardware bring-up test instead of the DitDah app" OFF)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    $<$<BOOL:${DITDAH_BRINGUP}>:DITDAH_BRINGUP>
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
