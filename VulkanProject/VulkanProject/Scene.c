#include "Scene.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Util.h"

void init_scene(Scene* scene)
{
	scene->camera.pos[0] = 0;
	scene->camera.pos[1] = 0;
	scene->camera.pos[2] = 3;
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

	flatten_scene(scene);

	return 0;
}
void flatten_scene(Scene* scene)
{
	SceneNode* node = &scene->scene_nodes[scene->scene_data.numSceneNodes - 1];

	FlatNodeResult result = flatten_node(scene, node, node);
	int a = 0;
	scene->scene_data.numTriangles = result.num_vertices / 3;
	scene->scene_data.numVertices = result.num_vertices;

	free(scene->vertices);
	scene->vertices = result.vertices;
	scene->indices = malloc(sizeof(uint32_t) * result.num_vertices);
	for(uint32_t i = 0;i < result.num_vertices;i++)
	{
		scene->indices[i] = i;
	}
}

FlatNodeResult flatten_node(Scene* scene, SceneNode* parent, SceneNode* node)
{
	FlatNodeResult result = { 0 };
	float tr[4][4];
	for (uint32_t r = 0; r != 4; r++) {
		for (uint32_t s = 0; s != 4; s++){
			tr[r][s] = parent->transform[r][0] * node->transform[0][s]
				+ parent->transform[r][1] * node->transform[1][s]
				+ parent->transform[r][2] * node->transform[2][s]
				+ parent->transform[r][3] * node->transform[3][s];
		}
	}

	// TODO multiply transforms
	if (node->NumTriangles > 0)
	{
		result.vertices = malloc(sizeof(Vertex) * node->NumTriangles * 3);
		result.num_vertices = node->NumTriangles * 3;
		for(int32_t i = 0;i <node->NumTriangles*3;i++)
		{
			Vertex v = scene->vertices[scene->indices[node->IndexBufferIndex + i]];
			float px = tr[0][0] * v.position[0] + tr[0][1] * v.position[1] + tr[0][2] * v.position[2] + tr[0][3];
			float py = tr[1][0] * v.position[0] + tr[1][1] * v.position[1] + tr[1][2] * v.position[2] + tr[1][3];
			float pz = tr[2][0] * v.position[0] + tr[2][1] * v.position[1] + tr[2][2] * v.position[2] + tr[2][3];

			result.vertices[i] = v; // TODO transform vertex
			result.vertices[i].position[0] = px;
			result.vertices[i].position[1] = py;
			result.vertices[i].position[2] = pz;
			int a = 1;
		}
	}

	if (node->NumChildren > 0)
	{
		for (int32_t i = 0;i<node->NumChildren;i++)
		{
			uint32_t child_idx = scene->node_indices[node->childrenIndex + i];
			SceneNode* child = &scene->scene_nodes[child_idx];

			FlatNodeResult childResult = flatten_node(scene, tr, child);

			Vertex* newVertArray = malloc(sizeof(Vertex) * (childResult.num_vertices + result.num_vertices));
			for (uint32_t j = 0;j<result.num_vertices;j++)
			{
				newVertArray[j] = result.vertices[j];
			}
			for (uint32_t j = 0; j < childResult.num_vertices; j++){
				newVertArray[result.num_vertices + j] = childResult.vertices[j];
			}
			free(childResult.vertices);
			free(result.vertices);
			
			result.num_vertices = result.num_vertices + childResult.num_vertices;
			result.vertices = newVertArray;
		}
	}

	return result;
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

