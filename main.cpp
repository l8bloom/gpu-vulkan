#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
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
  vk::raii::PhysicalDevice physicalDevice = NULL;
  std::vector<const char *> requiredDeviceExtension = {
      vk::KHRSwapchainExtensionName, vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName,
      vk::KHRCreateRenderpass2ExtensionName};

  static std::string
  stringifySeverityFlag(vk::DebugUtilsMessageSeverityFlagBitsEXT severity) {
    switch (severity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
      return "[ERROR]";
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
      return "[INFO]";
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
      return "[VERBOSE]";
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
      return "[WARNING]";
    }
  }

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
    std::cerr << stringifySeverityFlag(severity) << " validation layer: type "
              << to_string(type) << " msg: " << pCallbackData->pMessage
              << std::endl;

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

  void pickPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();

    if (devices.empty()) {
      throw std::runtime_error("No GPUs implementing Vulkan APIs found.");
    } else {
      std::cout << "Found " << devices.size() << " GPUs supporting Vulkan: ";
      for (const auto &dev : devices) {
        std::cout << dev.getProperties().deviceName;
        std::cout << ", ";
      }
      std::cout << std::endl;
    }

    for (auto const &device : devices) {
      auto queueFamilies = device.getQueueFamilyProperties();
      bool isSuitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
      // std::cout << "isSuitable: " << isSuitable << std::endl; // good
      auto qfpIter = std::ranges::find_if(
          queueFamilies, [](vk::QueueFamilyProperties const &qfp) {
            return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
                   static_cast<vk::QueueFlags>(0);
          });
      isSuitable = isSuitable && (qfpIter != queueFamilies.end());
      // std::cout << "isSuitable: " << isSuitable << std::endl; // good
      auto extensions = device.enumerateDeviceExtensionProperties();
      bool found = true;
      for (const auto &reqDevExt : requiredDeviceExtension) {
        auto extImplemented =
            std::ranges::any_of(extensions, [reqDevExt](const auto &extension) {
              return strcmp(reqDevExt, extension.extensionName) == 0;
            });
        found = found && extImplemented;
      }
      isSuitable = isSuitable && found;
      if (isSuitable) {
        std::cout << "Found a suitable device!"
                  << device.getProperties().deviceName << std::endl;
        physicalDevice = std::move(device);
        return;
      } else {
        std::cout << "Did NOT find a suitable device!" << std::endl;
      }
    }
    throw std::runtime_error("Didn't find any Vulkan devices which implement "
                             "required extensions :/\n");
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
    pickPhysicalDevice();
    std::cout << physicalDevice.getProperties().deviceName << std::endl;
    vk::QueueFamilyProperties pr;
    vk::QueueFlags qf;
    for (const auto &queuFam : physicalDevice.getQueueFamilyProperties()) {
      std::cout << "familiy has: " << queuFam.queueCount << "queus, ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eCompute)
        std::cout << "flags: " << "COMPUTE ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eGraphics)
        std::cout << "flags: " << "GRAPHICS ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eTransfer)
        std::cout << "flags: " << "MEMORY TRANSFER ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eVideoDecodeKHR)
        std::cout << "flags: " << "VIDEO DECODE ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eVideoEncodeKHR)
        std::cout << "flags: " << "VIDEO ENCODE ";
      std::cout << std::endl;
    }
    std::cout << "Number of Queue families: "
              << physicalDevice.getQueueFamilyProperties().size() << std::endl;
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
