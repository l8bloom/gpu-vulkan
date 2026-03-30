#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_wayland.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

int main(void) {

  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  VkInstance instance;
  VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Test",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  const char *const extensions[] = {
      "VK_KHR_surface",
      "VK_KHR_wayland_surface",
  };

  const char *const layers[] = {
      "VK_LAYER_KHRONOS_validation",
  };

  VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 1,
      .ppEnabledLayerNames = layers,
      .enabledExtensionCount = 2,
      .ppEnabledExtensionNames = extensions,
  };

  VkResult create_instance_result;

  if ((create_instance_result =
           vkCreateInstance(&createInfo, NULL, &instance)) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create Vulkan instance!\n");
    printf("VkResult code is: %d\n", create_instance_result);
    return -1;
  } else {
    printf("Vulkan Instance created OK!\n");
  }

  struct wl_display *display = glfwGetWaylandDisplay();
  GLFWwindow *window = glfwCreateWindow(1200, 800, "My Vulkan App", NULL, NULL);
  struct wl_surface *surface = glfwGetWaylandWindow(window);

  VkWaylandSurfaceCreateInfoKHR wl_create_info = {
      .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
      .flags = 0,
      .display = display,
      .surface = surface,
  };

  VkSurfaceKHR vk_surface;

  if (vkCreateWaylandSurfaceKHR(instance, &wl_create_info, NULL, &vk_surface) ==
      VK_SUCCESS) {
    printf("Wayland surface created OK!\n");
  } else {
    printf("Wayland surface NOT created!\n");
  }

  uint32_t gpus_count;

  VkResult res = vkEnumeratePhysicalDevices(instance, &gpus_count, NULL);
  printf("Number of gpus: %d\n", gpus_count);
  VkPhysicalDevice physical_device[gpus_count];
  VkResult res2 =
      vkEnumeratePhysicalDevices(instance, &gpus_count, physical_device);
  printf("Number of gpus: %d\n", gpus_count);

  /* uint32_t qu_families = 0; */
  /* vkGetPhysicalDeviceQueueFamilyProperties(physical_device[0], &qu_families,
   */
  /*                                          NULL); */

  /* VkQueueFamilyProperties phy_device_properties[qu_families]; */

  /* vkGetPhysicalDeviceQueueFamilyProperties(physical_device[0], &qu_families,
   */
  /*                                          phy_device_properties); */

  vkDestroySurfaceKHR(instance, vk_surface, NULL);
  vkDestroyInstance(instance, NULL);
  glfwTerminate();
}
