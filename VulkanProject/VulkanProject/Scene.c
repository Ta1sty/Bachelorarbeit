#include "Scene.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

	load_textures(&scene->texture_data, file);

	fclose(file);
	return 0;
}

void load_textures(TextureData* data, FILE* file)
{
	memset(data, 0, sizeof(TextureData));

	data->num_textures = 2;
	data->textures = malloc(sizeof(Texture) * data->num_textures);
	for (uint32_t i = 0; i< data->num_textures; i++)
	{
		load_texture(&data->textures[i], file);
	}
}
void load_texture(Texture* texture, FILE* file)
{
	int w = 2;
	int h = 2;
	texture->image_size = w * h * sizeof(uint32_t); // width * height * bytePerPixel
	texture->image_height = 2;
	texture->image_width = 2;
	texture->index = 0;
	texture->pixel_data = malloc(texture->image_size);

	texture->pixel_data[0] = 0x000000FF;
	texture->pixel_data[1] = 0x0000FF00;
	texture->pixel_data[2] = 0x00FF0000;
	texture->pixel_data[3] = 0x00000000;
}

