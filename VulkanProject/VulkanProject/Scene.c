#include "Scene.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Util.h"
#include <math.h>

#include "Globals.h"

void init_scene(Scene* scene)
{
	scene->camera.pos[0] = 0;
	scene->camera.pos[1] = 1;
	scene->camera.pos[2] = 3;
	scene->camera.rotation_x = 0;
	scene->camera.rotation_y = 0;

	load_scene(scene, "asd");
}

void load_scene(Scene* scene, char** path)
{
	FILE* file;
	fopen_s(&file, "dump.bin", "rb");

	if (!file)
		error("failed to open scene file");


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
}
void flatten_scene(Scene* scene)
{
	SceneNode* node = &scene->scene_nodes[scene->scene_data.numSceneNodes - 1];

	FlatNodeResult result = flatten_node(scene, node, node);
	int a = 0;
	scene->scene_data.numTriangles = result.num_vertices / 3;
	scene->scene_data.numVertices = result.num_vertices;

	free(scene->vertices);
	free(scene->indices);
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
		float normal[3][3];
		normal[0][0] = -tr[1][2] * tr[2][1] + tr[1][1] * tr[2][2];
		normal[0][1] = tr[1][2] * tr[2][0] - tr[1][0] * tr[2][2];
		normal[0][2] = -tr[1][1] * tr[2][0] + tr[1][0] * tr[2][1];
		normal[1][0] = tr[0][2] * tr[2][1] - tr[0][1] * tr[2][2];
		normal[1][1] = -tr[0][2] * tr[2][0] + tr[0][0] * tr[2][2];
		normal[1][2] = tr[0][1] * tr[2][0] - tr[0][0] * tr[2][1];
		normal[2][0] = -tr[0][2] * tr[1][1] + tr[0][1] * tr[1][2];
		normal[2][1] = tr[0][2] * tr[1][0] - tr[0][0] * tr[1][2];
		normal[2][2] = -tr[0][1] * tr[1][0] + tr[0][0] * tr[1][1];

		result.vertices = malloc(sizeof(Vertex) * node->NumTriangles * 3);
		result.num_vertices = node->NumTriangles * 3;
		for(int32_t i = 0;i <node->NumTriangles*3;i++)
		{
			Vertex v = scene->vertices[scene->indices[node->IndexBufferIndex + i]];
			
			result.vertices[i] = v;
			result.vertices[i].position[0] = tr[0][0] * v.position[0] + tr[0][1] * v.position[1] + tr[0][2] * v.position[2] + tr[0][3];
			result.vertices[i].position[1] = tr[1][0] * v.position[0] + tr[1][1] * v.position[1] + tr[1][2] * v.position[2] + tr[1][3];
			result.vertices[i].position[2] = tr[2][0] * v.position[0] + tr[2][1] * v.position[1] + tr[2][2] * v.position[2] + tr[2][3];

			result.vertices[i].normal[0] = normal[0][0] * v.normal[0] + normal[0][1] * v.normal[1] + normal[0][2] * v.normal[2];
			result.vertices[i].normal[1] = normal[1][0] * v.normal[0] + normal[1][1] * v.normal[1] + normal[1][2] * v.normal[2];
			result.vertices[i].normal[2] = normal[2][0] * v.normal[0] + normal[2][1] * v.normal[1] + normal[2][2] * v.normal[2];

			float invSqrt = sqrtf(result.vertices->normal[0] * result.vertices->normal[0]
				+ result.vertices->normal[1] * result.vertices->normal[1]
				+ result.vertices->normal[2] * result.vertices->normal[2]);
			invSqrt = 1 / invSqrt;
			result.vertices->normal[0] = invSqrt * result.vertices->normal[0];
			result.vertices->normal[1] = invSqrt * result.vertices->normal[1];
			result.vertices->normal[2] = invSqrt * result.vertices->normal[2];
		}
	}

	if (node->NumChildren > 0)
	{
		for (int32_t i = 0;i<node->NumChildren;i++)
		{
			uint32_t child_idx = scene->node_indices[node->childrenIndex + i];
			SceneNode* child = &scene->scene_nodes[child_idx];

			FlatNodeResult childResult = flatten_node(scene, node, child);

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

	// TODO create dummy materials so buffers have size > 0 if there are no textures
	fread(&data->num_materials, sizeof(uint32_t), 1, file);
	data->materials = malloc(sizeof(Material) * data->num_materials);
	fread(data->materials, sizeof(Material), data->num_materials, file);

	fread(&data->num_textures, sizeof(uint32_t), 1, file);
	//data->num_textures = 1;
	data->textures = malloc(sizeof(Texture) * data->num_textures);
	for (uint32_t i = 0; i< data->num_textures; i++)
	{
		load_texture(&data->textures[i], file);
	}
}
void load_texture(Texture* texture, FILE* file)
{
	
	memset(texture, 0, sizeof(Texture));
	fread(&texture->image_width, sizeof(uint32_t), 1, file);
	fread(&texture->image_height, sizeof(uint32_t), 1, file);
	fread(&texture->image_size, sizeof(uint32_t), 1, file);

	texture->pixel_data = malloc(texture->image_size);
	fread(texture->pixel_data, sizeof(char), texture->image_size, file);
	return;

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

void destroy_scene(Scene* scene)
{
	free(scene->indices);
	free(scene->node_indices);
	free(scene->vertices);
	free(scene->scene_nodes);
	free(scene->texture_data.materials);
	for (uint32_t i = 0; i < scene->texture_data.num_textures; i++)
	{
		free(scene->texture_data.textures[i].pixel_data);
	}
	free(scene->texture_data.textures);
}
