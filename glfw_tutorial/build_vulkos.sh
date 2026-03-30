#! /bin/bash

gcc main.c \
    -I/home/dom-ak45/Downloads/glfw/include \
    -L/home/dom-ak45/Downloads/glfw/build/src \
    -l glfw \
    -I/home/dom-ak45/vulkan/1.4.321.1/x86_64/include \
    -L/home/dom-ak45/vulkan/1.4.321.1/x86_64/lib \
    -l vulkan \
    -o vulkos

./vulkos
