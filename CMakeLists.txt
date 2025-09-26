cmake_minimum_required(VERSION 4.0...4.1 FATAL_ERROR)
project(vulkanized LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(vulkos main.cpp)
target_link_libraries(vulkos PRIVATE Vulkan::Vulkan glfw)

# debug: probe possible names and print properties
# foreach(t Vulkan::Vulkan Vulkan VulkanLoader VulkanLoader::VulkanLoader)
#   if(TARGET ${t})
#     message(STATUS "Found Vulkan target: ${t}")
#     get_target_property(_vk_inc ${t} INTERFACE_INCLUDE_DIRECTORIES)
#     get_target_property(_vk_lib ${t} IMPORTED_LOCATION)
#     message(STATUS "  include: ${_vk_inc}")
#     message(STATUS "  lib: ${_vk_lib}")
#   endif()
# endforeach()

# foreach(t glfw glfw3 glfw::glfw glfw3::glfw)
#   if(TARGET ${t})
#     message(STATUS "Found GLFW target: ${t}")
#     get_target_property(_g_inc ${t} INTERFACE_INCLUDE_DIRECTORIES)
#     get_target_property(_g_lib ${t} IMPORTED_LOCATION_NOCONFIG)
#     message(STATUS "  include: ${_g_inc}")
#     message(STATUS "  lib: ${_g_lib}")
#   endif()
# endforeach()
# foreach(path ${CMAKE_PREFIX_PATH})
#   message("Path: \n" ${path})
# endforeach(path)
