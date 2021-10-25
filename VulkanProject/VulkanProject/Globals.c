#include "Globals.h"

#include <stdio.h>

#include "Util.h"
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;

int err(const char* err)
{
	printf(err);  // NOLINT(clang-diagnostic-format-security)
	return FAILURE;
}
