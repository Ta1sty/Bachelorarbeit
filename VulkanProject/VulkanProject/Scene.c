#include "Scene.h"

#include <stdio.h>
#include <stdlib.h>

#include "Util.h"

void init_scene(Scene* scene)
{
	scene->camera.pos[0] = 0;
	scene->camera.pos[1] = 0;
	scene->camera.pos[2] = -2;
	scene->camera.rotation_x = 0;
	scene->camera.rotation_y = 0;

	load_scene(scene, "asd");
}

int load_scene(Scene* scene, char** path)
{
	FILE* file;
	fopen_s(&file, "dump.bin", "rb");

	if (!file)
		return FAILURE;


	fread(&scene->scene_data.numVertices, sizeof(uint32_t), 1, file);
	scene->vertices = malloc(sizeof(Vertex) * scene->scene_data.numVertices);
	fread(scene->vertices, sizeof(Vertex), scene->scene_data.numVertices, file); // TODO security check

	uint32_t numIndices;
	fread(&numIndices, sizeof(uint32_t), 1, file);
	scene->scene_data.numTriangles = numIndices / 3;
	scene->indices = malloc(sizeof(uint32_t) * numIndices);
	fread(scene->indices, sizeof(uint32_t), numIndices, file);

	fread(&scene->scene_data.numSceneNodes, sizeof(uint32_t), 1, file);
	scene->scene_nodes = malloc(sizeof(SceneNode) * scene->scene_data.numSceneNodes);
	fread(scene->scene_nodes, sizeof(SceneNode), scene->scene_data.numSceneNodes, file);

	fread(&scene->scene_data.numNodeIndices, sizeof(uint32_t), 1, file);
	scene->node_indices = malloc(sizeof(uint32_t) * scene->scene_data.numNodeIndices);
	fread(scene->node_indices, sizeof(uint32_t), scene->scene_data.numNodeIndices, file);



	fclose(file);
	return 0;
}
