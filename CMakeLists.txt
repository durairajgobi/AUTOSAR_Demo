# CMakeLists.txt
# Door Control System - CMake Build Configuration
# 
# Supports multiple build targets:
# - door_control_app: Main embedded application (firmware)
# - door_control_test: PC-based simulation and unit tests
#
# Usage:
#   mkdir build
#   cd build
#   cmake ..
#   make

cmake_minimum_required(VERSION 3.10)
project(DoorControlSystem C)

# Project version
set(PROJECT_VERSION 1.0.0)

# Compiler settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optional: Cross-compiler settings for embedded target
# Uncomment for ARM Cortex-M (e.g., STM32)
# set(CMAKE_C_COMPILER arm-none-eabi-gcc)
# set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
# set(CMAKE_OBJDUMP arm-none-eabi-objdump)
# set(CMAKE_SIZE arm-none-eabi-size)

# =============================================================================
# PROJECT STRUCTURE
# =============================================================================

# Source directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

# =============================================================================
# MAIN FIRMWARE BUILD (for embedded systems)
# =============================================================================

# Source files for firmware
set(FIRMWARE_SOURCES
    ${SOURCE_DIR}/DoorControl.h
    ${SOURCE_DIR}/06_SampleApplicationCode.c
    ${SOURCE_DIR}/Diagnostics.h
    ${SOURCE_DIR}/Diagnostics.c
    ${SOURCE_DIR}/Hal.c
    ${SOURCE_DIR}/main.c
)

# Header files
set(FIRMWARE_HEADERS
    ${SOURCE_DIR}/DoorControl.h
    ${SOURCE_DIR}/Diagnostics.h
)

# Create firmware library
add_library(door_control_lib STATIC ${FIRMWARE_SOURCES})
target_include_directories(door_control_lib PUBLIC ${INCLUDE_DIR})

# Optional: Set compiler flags for embedded build
# set_target_properties(door_control_lib PROPERTIES
#     C_FLAGS "-mcpu=cortex-m4 -mthumb -Wall -Wextra -O2"
# )

# Create firmware executable
add_executable(door_control_app ${FIRMWARE_SOURCES})
target_include_directories(door_control_app PUBLIC ${INCLUDE_DIR})
target_link_libraries(door_control_app door_control_lib)

# Generate hex and bin files (optional, for embedded systems)
# add_custom_command(TARGET door_control_app POST_BUILD
#     COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:door_control_app> 
#             ${BUILD_DIR}/door_control_app.hex
#     COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:door_control_app> 
#             ${BUILD_DIR}/door_control_app.bin
# )

# =============================================================================
# TEST BUILD (for PC-based simulation)
# =============================================================================

# Enable testing
enable_testing()

# Test sources
set(TEST_SOURCES
    ${SOURCE_DIR}/test_DoorControl.c
)

# Create test executable (for PC, no embedded code)
add_executable(door_control_test ${TEST_SOURCES})
target_include_directories(door_control_test PUBLIC ${INCLUDE_DIR})

# Test command
add_test(NAME DoorControlUnitTests COMMAND door_control_test)

# =============================================================================
# COMPILER OPTIONS
# =============================================================================

# Warning flags
set(COMMON_FLAGS "-Wall -Wextra -Wpedantic")

# Debug flags
set(DEBUG_FLAGS "-g -O0 -DDEBUG")

# Release flags
set(RELEASE_FLAGS "-O2 -DNDEBUG")

# Apply flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} ${DEBUG_FLAGS}")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} ${RELEASE_FLAGS}")
endif()

# =============================================================================
# BUILD OPTIONS
# =============================================================================

option(ENABLE_SIMULATION "Enable simulation mode (PC testing)" ON)
option(ENABLE_DIAGNOSTICS "Enable full diagnostics" ON)
option(BUILD_TESTS "Build unit tests" ON)

if(ENABLE_SIMULATION)
    add_compile_definitions(SIMULATION_MODE=1)
else()
    add_compile_definitions(SIMULATION_MODE=0)
endif()

if(ENABLE_DIAGNOSTICS)
    add_compile_definitions(ENABLE_DIAGNOSTICS=1)
else()
    add_compile_definitions(ENABLE_DIAGNOSTICS=0)
endif()

# =============================================================================
# OUTPUT CONFIGURATION
# =============================================================================

# Set output directories
set_target_properties(door_control_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${BUILD_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${BUILD_DIR}/lib
)

if(BUILD_TESTS)
    set_target_properties(door_control_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}/bin
    )
endif()

# =============================================================================
# DOCUMENTATION TARGETS
# =============================================================================

# Generate documentation (optional, requires doxygen)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in 
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc ALL
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )
endif()

# =============================================================================
# INSTALL TARGETS
# =============================================================================

install(TARGETS door_control_app
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${FIRMWARE_HEADERS}
    DESTINATION include
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    DESTINATION doc
)

# =============================================================================
# SUMMARY
# =============================================================================

message(STATUS "")
message(STATUS "Door Control System - Build Configuration")
message(STATUS "==========================================")
message(STATUS "Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "C Standard:           ${CMAKE_C_STANDARD}")
message(STATUS "Simulation mode:      ${ENABLE_SIMULATION}")
message(STATUS "Diagnostics enabled:  ${ENABLE_DIAGNOSTICS}")
message(STATUS "Build tests:          ${BUILD_TESTS}")
message(STATUS "")
message(STATUS "Targets to build:")
message(STATUS "  - door_control_app  : Main firmware application")
if(BUILD_TESTS)
    message(STATUS "  - door_control_test : PC-based unit tests")
    message(STATUS "")
    message(STATUS "Run tests with: make test")
endif()
message(STATUS "")

# =============================================================================
# BUILD INSTRUCTIONS
# =============================================================================

# Create build_instructions.txt
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/BUILD_INSTRUCTIONS.txt
"Door Control System - Build Instructions

1. Prerequisites:
   - CMake 3.10 or later
   - C compiler (gcc, clang, or ARM cross-compiler)
   - Optional: Doxygen (for documentation)

2. Configure and Build:
   mkdir build
   cd build
   cmake ..
   make

3. Run Tests (PC-based):
   make test
   # or
   ./bin/door_control_test

4. Build Options:
   cmake -DENABLE_SIMULATION=ON ..
   cmake -DBUILD_TESTS=ON ..
   cmake -DCMAKE_BUILD_TYPE=Debug ..

5. For Embedded System (ARM):
   # Uncomment cross-compiler settings in CMakeLists.txt
   cmake -DCMAKE_TOOLCHAIN_FILE=arm-cortex-m4.cmake ..
   make
   make install

6. Clean Build:
   rm -rf build
   mkdir build
   cd build
   cmake ..
   make

7. Installation:
   make install
   # or with custom prefix:
   cmake .. -DCMAKE_INSTALL_PREFIX=/opt/door_control
   make install
")

message(STATUS "See ${CMAKE_CURRENT_BINARY_DIR}/BUILD_INSTRUCTIONS.txt for details")
