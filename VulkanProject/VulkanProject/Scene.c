#include "Scene.h"

void init_scene(Scene* app)
{
	app->camera.pos[0] = 0;
	app->camera.pos[1] = 0;
	app->camera.pos[2] = -2;
	app->camera.rotation_x = 0;
	app->camera.rotation_y = 0;
}
