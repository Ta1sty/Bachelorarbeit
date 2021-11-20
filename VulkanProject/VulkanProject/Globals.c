#include "Globals.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "Util.h"
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
	printf("Type c to break execution\n");
	char c = getchar(); /* read a char */

	if(c == 'c')
	{
		int b = 0;
		int a = 1 / b;
	}
	printf("\n");
	if (exception_callback != NULL)
		exception_callback();

	exit(0);
}
