#! /bin/bash

set -e

# to not lose it:
# cmake -S . -B build -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_PREFIX_PATH="$HOME/.local;$HOME/Downloads/vulkanSDK/1.4.321.1/x86_64" -G Ninja
# GLFW:
# cmake -S . -B build -D CMAKE_C_COMPILER=clang -D CMAKE_INSTALL_PREFIX="$HOME/.local" -D GLFW_BUILD_X11=0 -D BUILD_SHARED_LIBS=ON -G Ninja

clang-tidy main.cpp

# this is not really needed to run every time, but doesn't matter
cmake \
    -S . \
    -B build \
    -D CMAKE_C_COMPILER=clang \
    -D CMAKE_CXX_COMPILER=clang++ \
    -D CMAKE_PREFIX_PATH="$HOME/.local;$HOME/Downloads/vulkanSDK/1.4.321.1/x86_64" \
    -G Ninja

ninja -C build

./build/vulkos
