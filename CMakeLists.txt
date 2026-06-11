cmake_minimum_required(VERSION 3.10)
project(GateSimulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compiler warnings for clean code verification
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/wire.cpp
    src/gate.cpp
    src/circuit.cpp
    src/parser.cpp
    src/main.cpp
)

add_executable(gate_simulator ${SOURCES})
