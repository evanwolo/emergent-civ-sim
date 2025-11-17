#!/bin/bash
# Build script for Linux/macOS

echo "Building Grand Strategy Simulation Engine..."
echo

mkdir -p build
cd build

# Try CMake if available
if command -v cmake &> /dev/null; then
    echo "Using CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
    if [ $? -eq 0 ]; then
        cmake --build .
        if [ $? -eq 0 ]; then
            echo
            echo "========================================"
            echo "Build successful!"
            echo "========================================"
            echo
            echo "Run the kernel simulation:"
            echo "  ./build/KernelSim"
            echo
            exit 0
        fi
    fi
fi

# Fallback to g++ if available
if command -v g++ &> /dev/null; then
    echo "Using g++ directly..."
    g++ -std=c++17 -O3 -march=native -Wall -Wextra -I../include \
        ../src/Kernel.cpp \
        ../src/KernelSnapshot.cpp \
        ../src/main_kernel.cpp \
        -o KernelSim
    if [ $? -eq 0 ]; then
        echo
        echo "========================================"
        echo "Build successful!"
        echo "========================================"
        echo
        echo "Run the kernel simulation:"
        echo "  ./build/KernelSim"
        echo
        exit 0
    fi
fi

echo "ERROR: No suitable compiler found!"
echo "Please install CMake or g++"
exit 1
