#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <vulkan/vulkan.hpp>
// clangd
#include "vulkan/vulkan_to_string.hpp"
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr uint32_t WINDOW_WIDTH = 2560;
constexpr uint32_t WINDOW_HEIGHT = 1440;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

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
    auto time_st = std::chrono::steady_clock::now();
    mainLoop();
    auto time_en = std::chrono::steady_clock::now();
    std::cout << "Main loop ran for: " << (time_en - time_st) / 1e9
              << std::endl;
    std::cout << " Number of frames: " << framesCount << std::endl;
    cleanup();
  }

private:
  double time_st, time_en;
  std::vector<std::chrono::nanoseconds> drawTimes;
  uint32_t currentFrame = 0;
  uint32_t semaphoreIndex = 0;
  uint64_t framesCount = 0;
  bool framebufferResized = false;
  GLFWwindow *window;
  vk::raii::Context context;
  vk::raii::Instance instance = NULL;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = NULL;
  vk::raii::PhysicalDevice physicalDevice = NULL;
  std::vector<const char *> requiredDeviceExtension = {
      vk::KHRSwapchainExtensionName,
      vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName,
      vk::KHRCreateRenderpass2ExtensionName,
      vk::KHRShaderDrawParametersExtensionName,
  };
  vk::raii::Device device = NULL;
  vk::raii::Queue graphicsQueue = NULL;
  vk::raii::Queue presentQueue = NULL;
  vk::raii::SurfaceKHR surface = NULL;
  vk::raii::SwapchainKHR swapChain = VK_NULL_HANDLE;
  std::vector<vk::Image> swapChainImages;
  vk::Format swapChainImageFormat = vk::Format::eUndefined;
  vk::Extent2D swapChainExtent;
  std::vector<vk::raii::ImageView> swapChainImageViews;
  vk::raii::PipelineLayout pipelineLayout = VK_NULL_HANDLE;
  vk::raii::Pipeline graphicsPipeline = VK_NULL_HANDLE;
  vk::raii::CommandPool commandPool = VK_NULL_HANDLE;
  std::vector<vk::raii::CommandBuffer> commandBuffers;
  std::vector<vk::raii::Semaphore> presentCompleteSemphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  std::vector<vk::raii::Fence> inFlightFences;

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

  static std::vector<char> readFile(const std::string &fileName) {
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error(std::format("Couldn't open the file", fileName));
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();
    return buffer;
  }

  void setupDebugMessenger() {
    if (!enableValidationLayers)
      return;

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

  vk::Extent2D
  _chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
    int imageExtentHeight, imageExtentWidth;

    if (surfaceCapabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max())
      return surfaceCapabilities.currentExtent;

    glfwGetFramebufferSize(window, &imageExtentWidth, &imageExtentHeight);
    std::cout << "Final image width: " << imageExtentWidth
              << ", final image height: " << imageExtentHeight << std::endl;

    return {std::clamp<uint32_t>(imageExtentHeight,
                                 surfaceCapabilities.minImageExtent.height,
                                 surfaceCapabilities.maxImageExtent.height),
            std::clamp<uint32_t>(imageExtentWidth,
                                 surfaceCapabilities.minImageExtent.width,
                                 surfaceCapabilities.maxImageExtent.width)};
  }

  vk::SurfaceFormatKHR _chooseSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    for (auto format : availableFormats) {
      std::cout << vk::to_string(format.format) << " "
                << vk::to_string(format.colorSpace) << std::endl;
    }

    vk::SurfaceFormatKHR chosenFormat = availableFormats[0];
    for (auto &format : availableFormats) {
      if ((format.format == vk::Format::eB8G8R8A8Srgb) &&
          (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)) {
        chosenFormat = format;
        break;
      }
    }

    std::cout << "Chosen format: " << vk::to_string(chosenFormat.format)
              << ", chosen color space: "
              << vk::to_string(chosenFormat.colorSpace) << std::endl;

    return chosenFormat;
  }

  vk::PresentModeKHR _choosePresentMode(
      const std::vector<vk::PresentModeKHR> &availablePresentModes) {

    std::cout << "Available present modes count: "
              << availablePresentModes.size() << std::endl;

    for (auto presentMode : availablePresentModes) {
      if (presentMode == vk::PresentModeKHR::eFifo)
        std::cout << "Fifo supported" << std::endl;
      if (presentMode == vk::PresentModeKHR::eFifoLatestReady)
        std::cout << "Fifo Latest Ready supported" << std::endl;

      if (presentMode == vk::PresentModeKHR::eFifoRelaxed)
        std::cout << "Fifo Relaxed supported" << std::endl;

      if (presentMode == vk::PresentModeKHR::eImmediate)
        std::cout << "Immediate supported" << std::endl;

      if (presentMode == vk::PresentModeKHR::eMailbox)
        std::cout << "Emailbox supported" << std::endl;

      if (presentMode == vk::PresentModeKHR::eSharedContinuousRefresh)
        std::cout << "Shared supported" << std::endl;

      if (presentMode == vk::PresentModeKHR::eSharedDemandRefresh)
        std::cout << "Shared Demand supported" << std::endl;
    }

    vk::PresentModeKHR chosenPresentMode = vk::PresentModeKHR::eFifo;
    for (auto &presentMode : availablePresentModes) {
      if (presentMode == vk::PresentModeKHR::eMailbox) {
        chosenPresentMode = presentMode;
        break;
      }
    }

    std::cout << "Chosen present mode is "
              << (chosenPresentMode == vk::PresentModeKHR::eMailbox
                      ? "Mailbox(tripple buffering) mode"
                      : "Fifo mode")
              << std::endl;
    return chosenPresentMode;
  }

  void createSwapChain() {
    auto surfaceCapabilities =
        physicalDevice.getSurfaceCapabilitiesKHR(surface);

    swapChainExtent = _chooseSwapExtent(surfaceCapabilities);
    auto swapChainSurfaceFormat =
        _chooseSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
    swapChainImageFormat = swapChainSurfaceFormat.format;
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    minImageCount = (surfaceCapabilities.maxImageCount > 0 &&
                     minImageCount > surfaceCapabilities.maxImageCount)
                        ? surfaceCapabilities.maxImageCount
                        : minImageCount;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = _choosePresentMode(
            physicalDevice.getSurfacePresentModesKHR(surface)),
        .clipped = vk::True,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    // NOTE:
    // in tutorial they have checked if the graphics and present queues are
    // different, if so swap chain create info needs to be aware of it and of
    // the ownership. The tutorial does say that most of the time on most of the
    // hardware the two queues will be under the same family, I will just
    // hardcode the known family
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = 0;
    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();

    std::cout << "Number of swap chain images: " << swapChainImages.size()
              << std::endl
              << "Max number of image layers: "
              << surfaceCapabilities.maxImageArrayLayers << std::endl
              << "Current transform value returned by the surface(WSI): "
              << vk::to_string(surfaceCapabilities.currentTransform)
              << std::endl;
  }

  void createLogicalDevice() {
    uint32_t graphicsFamilyIndex = UINT32_MAX, counter = 0;
    for (const auto &queuFam : physicalDevice.getQueueFamilyProperties()) {
      std::cout << "familiy has: " << queuFam.queueCount << "queus, ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eCompute)
        std::cout << "flags: " << "COMPUTE ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eGraphics) {
        std::cout << "flags: " << "GRAPHICS ";
        // it turns out that a graphics queue family does not have to
        // necessarily support the presentation!
        // actually same queue family which supports drawing does not have to
        // support presentation
        vk::Bool32 surfaceSupported =
            physicalDevice.getSurfaceSupportKHR(counter, surface);
        if (surfaceSupported)
          graphicsFamilyIndex = counter;
      }
      if (queuFam.queueFlags & vk::QueueFlagBits::eTransfer)
        std::cout << "flags: " << "MEMORY TRANSFER ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eVideoDecodeKHR)
        std::cout << "flags: " << "VIDEO DECODE ";
      if (queuFam.queueFlags & vk::QueueFlagBits::eVideoEncodeKHR)
        std::cout << "flags: " << "VIDEO ENCODE ";
      std::cout << std::endl;
      counter++;
    }
    if (graphicsFamilyIndex == UINT32_MAX)
      throw std::runtime_error(
          "Couldn't find appropriate Queue familiy, terminating\n");
    std::cout << "Number of Queue families: "
              << physicalDevice.getQueueFamilyProperties().size() << std::endl;
    std::cout << "Graphixs Family Index: " << graphicsFamilyIndex << std::endl;
    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain{{},
                     {
                         .synchronization2 = true,
                         .dynamicRendering = true,
                     },
                     {
                         .extendedDynamicState = true,
                     }};

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data(),
    };
    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    // Could be: device.getQueue(uint32_t queueFamilyIndex, uint32_t queueIndex)
    graphicsQueue = vk::raii::Queue(device, graphicsFamilyIndex, 0);
    // assume present queue will be the same as graphics queue, ususally it
    // needs to be queried for
    presentQueue = vk::raii::Queue(device, graphicsFamilyIndex, 0);
  }

  void createSurface() {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, NULL, &_surface) !=
        VK_SUCCESS) {
      throw std::runtime_error("Error when creating the Vulkan surface.\n");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
  }

  void createImageViews() {
    swapChainImageViews.clear();
    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainImageFormat,
        .components =
            vk::ComponentMapping{
                .r = vk::ComponentSwizzle::eR,
                .g = vk::ComponentSwizzle::eG,
                .b = vk::ComponentSwizzle::eB,
                .a = vk::ComponentSwizzle::eA,
            },
        .subresourceRange =
            vk::ImageSubresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,

            },
    };
    for (const auto &image : swapChainImages) {
      imageViewCreateInfo.image = image;
      swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
  }

  void createGraphicsPipeline() {
    auto shaderModule = createShaderModule(readFile("shaders/slang.spv"));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain",
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain",
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo,
    };

    vk::PipelineVertexInputStateCreateInfo vertexinputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList,
    };
    vk::PipelineViewportStateCreateInfo viewPortState{
        .viewportCount = 1,
        .scissorCount = 1,
    };
    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f,
    };
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };
    // happens after the fragment shader
    // Common use cases: transparency, additive glow, particle effects, etc.
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    std::vector dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainImageFormat,
    };
    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexinputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewPortState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = VK_NULL_HANDLE,
    };
    graphicsPipeline = vk::raii::Pipeline(device, VK_NULL_HANDLE, pipelineInfo);
  }

  [[nodiscard]] vk::raii::ShaderModule
  createShaderModule(const std::vector<char> &code) {
    auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t *>(code.data())};

    return vk::raii::ShaderModule(device, shaderModuleCreateInfo);
  }

  void createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = 0,
    };
    commandPool = vk::raii::CommandPool(device, poolInfo);
  }

  void createCommandBuffers() {
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };
    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
  }

  void recordCommandBuffer(uint32_t imageIndex) {
    commandBuffers[currentFrame].begin({});
    transition_image_layout(imageIndex, vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal, {},
                            vk::AccessFlagBits2::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits2::eTopOfPipe,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    // vk::ClearValue clearColor = vk::ClearColorValue(0.5f, 0.5f, 0.5f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor,
    };
    vk::RenderingInfo renderingInfo = {
        .renderArea =
            {
                .offset = {0, 0},
                .extent = swapChainExtent,
            },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo,
    };
    commandBuffers[currentFrame].beginRendering(renderingInfo);
    commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics,
                                              graphicsPipeline);
    commandBuffers[currentFrame].setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width),
                        static_cast<float>(swapChainExtent.height)));
    commandBuffers[currentFrame].setScissor(
        0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
    // fucking finally
    commandBuffers[currentFrame].draw(6, 1, 0, 0);
    commandBuffers[currentFrame].endRendering();
    transition_image_layout(imageIndex,
                            vk::ImageLayout::eColorAttachmentOptimal,
                            vk::ImageLayout::ePresentSrcKHR,
                            vk::AccessFlagBits2::eColorAttachmentWrite, {},
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits2::eBottomOfPipe);
    commandBuffers[currentFrame].end();
  }

  void transition_image_layout(uint32_t imageIndex, vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               vk::AccessFlags2 srcAccessMask,
                               vk::AccessFlags2 dstAccessMask,
                               vk::PipelineStageFlags2 srcStageMask,
                               vk::PipelineStageFlags2 dstStageMask) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChainImages[imageIndex],
        .subresourceRange =
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    vk::DependencyInfo dependencyInfo{
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };
    commandBuffers[currentFrame].pipelineBarrier2(dependencyInfo);
  }

  void drawFrame() {
    auto draw_st = std::chrono::steady_clock::now();
    while (vk::Result::eTimeout ==
           device.waitForFences(*inFlightFences[currentFrame], vk::True,
                                UINT64_MAX))
      ;
    auto [result, imageIndex] = swapChain.acquireNextImage(
        UINT64_MAX, presentCompleteSemphores[semaphoreIndex], nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR) {
      std::cout << "Will recreate swap chain to acquire new image."
                << std::endl;

      recreateSwapChain();
      return;
    }
    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error(
          "Failed to acquired a swap chain image! Terminating.");
    }
    commandBuffers[currentFrame].reset();
    recordCommandBuffer(imageIndex);
    device.resetFences(
        *inFlightFences[currentFrame]); // resetting = unsignalling
    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemphores[semaphoreIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores[semaphoreIndex],
    };
    graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[semaphoreIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain,
        .pImageIndices = &imageIndex,
    };
    result = presentQueue.presentKHR(presentInfoKHR);
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR || framebufferResized) {
      std::cout << "Will recreate swap chain for optimal presentation"
                << std::endl;
      framebufferResized = false;
      recreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
      throw std::runtime_error(
          "Error when queueing swap image for presentation! Terminating.");
    }

    semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemphores.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    auto draw_en = std::chrono::steady_clock::now();
    drawTimes.emplace_back(draw_en - draw_st);
  }

  void createSyncObjects() {
    presentCompleteSemphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();
    for (int i = 0; i < swapChainImages.size(); i++) {
      presentCompleteSemphores.emplace_back(device, vk::SemaphoreCreateInfo());
      renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
    }
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      inFlightFences.emplace_back(
          device,
          vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
  }

  void cleanupSwaphain() {
    swapChainImageViews.clear();
    swapChain = nullptr;
  }

  void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
      std::cout << "width or height became 0" << std::endl;
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
    }
    device.waitIdle();
    cleanupSwaphain();
    createSwapChain();
    createImageViews();
  }

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height) {
    auto app = reinterpret_cast<HelloTriangleApplication *>(
        glfwGetWindowUserPointer(window));
    std::cout << "Frame buffer resized" << std::endl;
    app->framebufferResized = true;
  }

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Triangle",
                              NULL, NULL);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  }

  void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
  }

  void mainLoop() {
    // while (!(glfwWindowShouldClose(window)) && drawTimes.size() < 100000) {
    while (!(glfwWindowShouldClose(window))) {
      glfwPollEvents();
      drawFrame();
      framesCount++;
      // std::cout << drawTimes.size() << std::endl;
    }
    device.waitIdle();
  }

  void cleanup() {
    // for (auto it : drawTimes) {
    //   std::cout << std::scientific << std::setprecision(2)
    //             << std::chrono::duration<double>(it).count() << std::endl;
    // }
    cleanupSwaphain();

    // NOTE: this is too early to release the window, it seems that
    // wayland compositor snaps and the windowing system freezes here, since the
    // object will on destructing do a 2nd free() of the wayland surface
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

int main() {
  HelloTriangleApplication app;
  try {
    app.run();
    // throw std::runtime_error("Smth went wrong on purpose");
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
