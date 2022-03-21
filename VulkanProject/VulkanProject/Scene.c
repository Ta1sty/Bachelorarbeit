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
	scene->camera.pos[1] = 0;
	scene->camera.pos[2] = 4;
	scene->camera.rotation_x = 0;
	scene->camera.rotation_y = 0;
	scene->camera.settings.fov = 45;
	scene->camera.settings.colorSensitivity = 15;
	scene->camera.settings.textures = 1;
	scene->camera.settings.ambient = 1;
	scene->camera.settings.diffuse = 1;
	scene->camera.settings.specular = 1;
	scene->camera.settings.shadows = 1;
	scene->camera.settings.reflection = 1;
	scene->camera.settings.transmission = 1;
	scene->camera.settings.maxDepth = 5;
	scene->camera.settings.traceMax = 100;
	scene->camera.settings.pixelX = WINDOW_WIDTH/2;
	scene->camera.settings.pixelY = WINDOW_HEIGHT/2;
}

void load_scene(Scene* scene, char* path)
{
	//int vert = system("buildScene.bat");

	size_t size = sizeof(char) * 256;
	char* buffer = malloc(size);
	strcpy_s(buffer, size, "../Scenes/");
	strcat_s(buffer, size, path);
	strcat_s(buffer, size, ".vksc");
	FILE* file;
	fopen_s(&file, buffer, "rb");

	if (!file)
		error("failed to open scene file");
	free(buffer);

	// VertexBuffer
	fread(&scene->scene_data.numVertices, sizeof(uint32_t), 1, file);
	scene->vertices = malloc(sizeof(Vertex) * scene->scene_data.numVertices);
	fread(scene->vertices, sizeof(Vertex), scene->scene_data.numVertices, file); // TODO security check

	// IndexBuffer
	uint32_t numIndicesF;
	fread(&numIndicesF, sizeof(uint32_t), 1, file);
	scene->scene_data.numTriangles = numIndicesF / 3;
	scene->indices = malloc(sizeof(uint32_t) * numIndicesF);
	fread(scene->indices, sizeof(uint32_t), numIndicesF, file);

	// SceneNodes
	fread(&scene->scene_data.numSceneNodes, sizeof(uint32_t), 1, file);
	fread(&scene->scene_data.rootSceneNode, sizeof(uint32_t), 1, file);

	scene->scene_nodes = malloc(sizeof(SceneNode) * scene->scene_data.numSceneNodes);
	fread(scene->scene_nodes, sizeof(SceneNode), scene->scene_data.numSceneNodes, file);

	// transforms
	fread(&scene->scene_data.numTransforms, sizeof(uint32_t), 1, file);
	scene->node_transforms = malloc(sizeof(Mat4x3) * scene->scene_data.numTransforms);
	fread(scene->node_transforms, sizeof(Mat4x3), scene->scene_data.numTransforms, file);

	//free(buffer);
	// SceneNode children
	fread(&scene->scene_data.numNodeIndices, sizeof(uint32_t), 1, file);
	scene->node_indices = malloc(sizeof(uint32_t) * scene->scene_data.numNodeIndices);
	fread(scene->node_indices, sizeof(uint32_t), scene->scene_data.numNodeIndices, file);


	// init a light source
	scene->scene_data.numLights = 1;
	scene->lights = malloc(sizeof(Light) * scene->scene_data.numLights);
	Light light1 = {
		.position = {0,3,2},
		.type = LIGHT_ON | LIGHT_TYPE_SUN,
		.intensity = {1,1,0.7f},
		.maxDst = 30,
		.quadratic = {2,0,0},
		.direction = {-1,-0.5f,-1},
	};
	//scene->lights[0] = light1;
	scene->lights[0] = light1;
	load_textures(&scene->texture_data, file);
	fclose(file);

	init_scene(scene);
}

void load_textures(TextureData* data, FILE* file)
{
	memset(data, 0, sizeof(TextureData));

	fread(&data->num_materials, sizeof(uint32_t), 1, file);
	if(data->num_materials != 0)
	{
		data->materials = malloc(sizeof(Material) * data->num_materials);
		fread(data->materials, sizeof(Material), data->num_materials, file);
	} else // create dummy material so buffer has size > 0
	{
		data->num_materials = 1;
		data->materials = malloc(sizeof(Material));
		data->materials[0].k_a = 0.3f;
		data->materials[0].k_d = 0.4f;
		data->materials[0].k_s = 0.3f;
		data->materials[0].texture_index = 0;
	}

	fread(&data->num_textures, sizeof(uint32_t), 1, file);
	if(data->num_textures != 0)
	{
		data->textures = malloc(sizeof(Texture) * data->num_textures);
		for (uint32_t i = 0; i < data->num_textures; i++)
		{
			load_texture(&data->textures[i], file);
		}
	} else // create dummy texture so buffer has size > 0
	{
		data->num_textures = 1;
		data->textures = malloc(sizeof(Texture));
		data->textures[0].image_size = sizeof(uint32_t); // width * height * bytePerPixel
		data->textures[0].image_height = 1;
		data->textures[0].image_width = 1;
		data->textures[0].index = 0;
		data->textures[0].pixel_data = malloc(data->textures[0].image_size);
		data->textures[0].pixel_data[0] = 0xFFFF00FF;
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
}

void destroy_scene(Scene* scene)
{
	free(scene->indices);
	free(scene->node_indices);
	free(scene->vertices);
	free(scene->scene_nodes);
	free(scene->texture_data.materials);
	free(scene->lights);
	for (uint32_t i = 0; i < scene->texture_data.num_textures; i++)
	{
		free(scene->texture_data.textures[i].pixel_data);
	}
	free(scene->texture_data.textures);
	memset(scene, 0, sizeof(Scene));
}