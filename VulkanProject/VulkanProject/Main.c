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
int resizeW = -1;
int resizeH = -1;

void resize_callback(GLFWwindow* window, int width, int height) {
    resizeW = width;
    resizeH = height;
}



int main()
{
    App app;
    memset(&app, 0, sizeof(app));
    App* appPtr = &app;
    if(init_window(&app.window) && init_vulkan(&app.vk_info, &app.window))
    {
        glfwSetFramebufferSizeCallback(app.window, resize_callback);
        while (!glfwWindowShouldClose(app.window)) {
            glfwPollEvents();
            if(WINDOW_WIDTH != 0 && WINDOW_HEIGHT != 0)
            {
                if (drawFrame(&app.vk_info) == FAILURE)
                {
                    break;
                }
            }

            if (resizeW >= 0 || resizeH >= 0)
            {
                create_or_resize_swapchain(&app.vk_info, &app.window, resizeW, resizeH);
                WINDOW_WIDTH = resizeW;
                WINDOW_HEIGHT = resizeH;
                resizeW = -1;
                resizeH = -1;
            }
        }
        destroy_vulkan(&app.vk_info);
        destroy_window(app.window);
        int a = 5;
        return 0;
    }
    printf("Init failed\n");
    destroy_vulkan(&app.vk_info);
    destroy_window(app.window);
    return -1;
}
 