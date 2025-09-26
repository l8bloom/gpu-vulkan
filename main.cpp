#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;

const std::vector validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
constexpr auto enableValidationLayers = false;
#else
constexpr auto enableValidationLayers = true;
#endif

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
  vk::raii::Instance instance = NULL;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = NULL;

  void setupDebugMessenger() {
    if (!enableValidationLayers) {
      return;
    }
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback};
    debugMessenger =
        instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
  }

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    std::cerr << "validation layer: type " << to_string(type)
              << " msg: " << pCallbackData->pMessage << std::endl;

    return vk::False;
  }

  std::vector<const char *> getLayers() {
    std::vector<char const *> requiredLayers;
    if (enableValidationLayers) {
      requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }
    auto layerProperties = context.enumerateInstanceLayerProperties();
    // checks if any required layer is not implemented by the ICD
    if (std::ranges::any_of(
            requiredLayers, [&layerProperties](auto const &reqLayer) {
              return std::ranges::none_of(
                  layerProperties, [reqLayer](auto const &layerProperty) {
                    return strcmp(layerProperty.layerName, reqLayer) == 0;
                  });
            })) {

      throw std::runtime_error("A validation is not supported");
    }
    return requiredLayers;
  }

  std::vector<const char *> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions,
                                         glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
      extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (auto v : extensionProperties) {
      std::cout << '\t' << std::string(v.extensionName) << std::endl;
    }
    for (auto ext : extensions) {
      if (std::ranges::none_of(
              extensionProperties, [ext](auto const &extensionProperty) {
                return strcmp(extensionProperty.extensionName, ext) == 0;
              })) {
        throw std::runtime_error("Required GLFW extension not supported: " +
                                 std::string(ext));
      }
    }
    std::cout << "All required extensions implemented. ✅\n";
    return extensions;
  }

  void createInstance() {
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "My Vulkan Triangle",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName = "No engine",
        .apiVersion = vk::ApiVersion14,
    };
    auto requiredLayers = getLayers();
    auto requiredExtensions = getRequiredExtensions();
    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount =
            static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),

    };
    instance = vk::raii::Instance(context, createInfo);

    if (enableValidationLayers) {
      std::cout << "Validations ON" << std::endl;
    } else {
      std::cout << "Validations OFF" << std::endl;
    }
  }
  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Triangle",
                              NULL, NULL);
  }
  void initVulkan() {
    createInstance();
    setupDebugMessenger();
  }
  void mainLoop() {
    while (!(glfwWindowShouldClose(window))) {
      glfwWaitEvents();
      // glfwPollEvents();
      //  printf("Yes: %d\n", glfwVulkanSupported());
      //  return;
      //  glfwCreateWindowSurface
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
