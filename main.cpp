#include <cstddef>
#include <cstdint>
#include <cstdio>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;

class HelloTriangleApplication {
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  GLFWwindow *window;
  vk::raii::Context context;
  // vk::raii::Instance instance;
  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Triangle",
                              NULL, NULL);
  }
  void initVulkan() {
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "My Vulkan Triangle",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName = "No engine",
        .apiVersion = vk::ApiVersion14,
    };
  }
  void mainLoop() {
    while (!(glfwWindowShouldClose(window))) {
      glfwPollEvents();
      // printf("Yes: %d\n", glfwVulkanSupported());
      // return;
      // glfwCreateWindowSurface
    }
  }
  void cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

int main() {
  HelloTriangleApplication app;
  try {
    app.run();
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
