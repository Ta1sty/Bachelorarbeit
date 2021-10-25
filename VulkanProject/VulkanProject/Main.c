// VulkanProject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#include <string.h>

#include "Vulkan.h"
#include "Window.h"
#include "Globals.h"
#include "Util.h"

int main()
{
    App app;
    memset(&app, 0, sizeof(app));
    App* appPtr = &app;
    if(init_window(&app.window) && init_vulkan(&app.vk_info, &app.window))
    {
        while (!glfwWindowShouldClose(app.window)) {
            glfwPollEvents();
            if(drawFrame(&app.vk_info) == FAILURE)
            {
                break;
            }
        }
        destroy_vulkan(&app.vk_info);
        destroy_window(app.window);
        return 0;
    }
    printf("Init failed\n");
    destroy_vulkan(&app.vk_info);
    destroy_window(app.window);
    return -1;
}
 