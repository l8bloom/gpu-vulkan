#! /bin/bash

set -e


slangc shaders/particles.slang \
       -target spirv \
       -profile spirv_1_4 \
       -emit-spirv-directly \
       -fvk-use-entrypoint-name \
       -entry vertexMain \
       -entry fragmentMain \
       -entry computeMain \
       -o shaders/particles.spv

clang-tidy particles.cpp

# this is not really needed to run every time, but doesn't matter
if [[ "$1" == "release" ]]; then
    rm -rf ./build
    cmake \
	-S . \
	-B build \
	-D CMAKE_C_COMPILER=clang \
	-D CMAKE_CXX_COMPILER=clang++ \
	-D CMAKE_PREFIX_PATH="$HOME/.local;$HOME/Downloads/vulkanSDK/1.4.321.1/x86_64" \
	-D CMAKE_BUILD_TYPE=Release \
	-D SHADER_SLANG_SOURCES="shader.slang" \
	-G Ninja
else
    rm -rf ./build    
    cmake \
        -S . \
	-B build \
	-D CMAKE_C_COMPILER=clang \
	-D CMAKE_CXX_COMPILER=clang++ \
	-D CMAKE_PREFIX_PATH="$HOME/.local;$HOME/Downloads/vulkanSDK/1.4.321.1/x86_64" \
	-D SHADER_SLANG_SOURCES="shader.slang" \
	-G Ninja
fi

ninja -C build

./build/particles
