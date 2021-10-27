#include "Window.h"

#include <stdio.h>
#include <GLFW/glfw3.h>
#include "Util.h"
#include "Globals.h"

int init_window(GLFWwindow** window)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", NULL, NULL);
	if (window == NULL) {
		printf("Failed to create Window\n");
		return FAILURE;
	}
	return SUCCESS;
}

void destroy_window(GLFWwindow* window)
{
	if(window != NULL) glfwDestroyWindow(window);
	glfwTerminate();
}
