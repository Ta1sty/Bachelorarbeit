#include "Util.h"

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

int SUCCESS = 1;
int FAILURE = 0;
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;
ExceptionCallback exception_callback = NULL;

void setExceptionCallback(ExceptionCallback callback)
{
	exception_callback = callback;
}

void check(VkResult result, char* errorMsg)
{
	if (result == VK_SUCCESS) return;
	if (errorMsg[0] == '\0') errorMsg = "Result does not indicate success";
	error(errorMsg);
}

void check_b(VkBool32 boolean, char* errorMsg)
{
	if (boolean == VK_TRUE) return;
	if (errorMsg[0] == '\0') errorMsg = "Result does not indicate success";
	error(errorMsg);
}

void error(char* err) {
	printf(err);
	printf("\n");
	char c;
#ifdef NDEBUG
	printf("Type c to break execution\n");
	c = getchar(); /* read a char */
#else
	c = 'c';
#endif
	if (c == 'c')
	{
		int b = 0;
		int a = 1 / b;
	}
	printf("\n");
	if (exception_callback != NULL)
		exception_callback();

	exit(0);
}