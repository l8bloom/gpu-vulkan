#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

class HelloTriangleApplication {
public:
  void run() {
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  vk::raii::Context context;
  void initVulkan() {
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "My Vulkan Triangle",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName = "No engine",
        .apiVersion = vk::ApiVersion14,
    };
  }
  void mainLoop() {}
  void cleanup() {}
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
