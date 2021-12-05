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
	scene->camera.fov = 45;
	load_scene(scene, "asd");
}

void load_scene(Scene* scene, char** path)
{
	int vert = system("buildScene.bat");

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
	SceneNodeData* buffer = malloc(sizeof(SceneNodeData) * scene->scene_data.numSceneNodes);
	fread(buffer, sizeof(SceneNodeData), scene->scene_data.numSceneNodes, file);

	scene->scene_nodes = malloc(sizeof(SceneNode) * scene->scene_data.numSceneNodes);
	memset(scene->scene_nodes, 0, sizeof(SceneNode) * scene->scene_data.numSceneNodes);

	for (uint32_t i = 0; i < scene->scene_data.numSceneNodes; i++)
	{
		scene->scene_nodes[i].data = buffer[i];
	}

	fread(&scene->scene_data.numNodeIndices, sizeof(uint32_t), 1, file);
	scene->node_indices = malloc(sizeof(uint32_t) * scene->scene_data.numNodeIndices);
	fread(scene->node_indices, sizeof(uint32_t), scene->scene_data.numNodeIndices, file);
	
	load_textures(&scene->texture_data, file);
	fclose(file);
}
void flatten_scene(Scene* scene)
{
	SceneNode* node = &scene->scene_nodes[scene->scene_data.numSceneNodes - 1];

	float id[4][4] = {
		{1,0,0,0},
		{0,1,0,0},
		{0,0,1,0},
		{0,0,0,1}
	};

	FlatNodeResult result = flatten_node(scene, id, node);
	int a = 0;
	scene->scene_data.numTriangles = result.num_vertices / 3;
	scene->scene_data.numVertices = result.num_vertices; // Maybe vertex dedublication

	free(scene->vertices);
	free(scene->indices);
	scene->vertices = result.vertices;
	scene->indices = malloc(sizeof(uint32_t) * result.num_vertices);
	for(uint32_t i = 0;i < result.num_vertices;i++)
	{
		scene->indices[i] = i;
	}

	free(scene->scene_nodes);
	free(scene->node_indices);
	scene->scene_nodes = malloc(sizeof(SceneNode));
	SceneNode everything = {
		.data.childrenIndex = -1,
		.data.Index = 0,
		.data.IndexBufferIndex = 0,
		.data.NumTriangles = scene->scene_data.numTriangles,
		.data.NumChildren = 0
	};
	scene->scene_nodes[0] = everything;
	memcpy(scene->scene_nodes[0].data.transform, id, sizeof(float) * 4 * 4);
	scene->node_indices = malloc(sizeof(uint32_t));
	scene->scene_data.numNodeIndices = 1;
	scene->scene_data.numSceneNodes = 1;
}

FlatNodeResult flatten_node(Scene* scene, float parentTransform[4][4], SceneNode* node)
{
	FlatNodeResult result = { 0 }; // multi level should be working now!, missing passthrough transform
	float tr[4][4];
	for (uint32_t r = 0; r != 4; r++) {
		for (uint32_t s = 0; s != 4; s++){
			tr[r][s] = parentTransform[r][0] * node->data.transform[0][s]
				+ parentTransform[r][1] * node->data.transform[1][s]
				+ parentTransform[r][2] * node->data.transform[2][s]
				+ parentTransform[r][3] * node->data.transform[3][s];
		}
	}

	if (node->data.NumTriangles > 0)
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

		result.vertices = malloc(sizeof(Vertex) * node->data.NumTriangles * 3);
		result.num_vertices = node->data.NumTriangles * 3;
		for(int32_t i = 0;i <node->data.NumTriangles*3;i++)
		{
			Vertex v = scene->vertices[scene->indices[node->data.IndexBufferIndex + i]];
			
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

	if (node->data.NumChildren > 0)
	{
		for (int32_t i = 0;i<node->data.NumChildren;i++)
		{
			uint32_t child_idx = scene->node_indices[node->data.childrenIndex + i];
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
	for (uint32_t i = 0; i < scene->texture_data.num_textures; i++)
	{
		free(scene->texture_data.textures[i].pixel_data);
	}
	free(scene->texture_data.textures);
}
// collapses all scene Nodes that do not reference any geometry, after that only the last node references children, the rest only references geometry
void collapse_parent_nodes(Scene* scene)
{
	SceneNode root = scene->scene_nodes[scene->scene_data.numSceneNodes - 1];
	if (root.data.NumChildren < 0) return;

	NodeCollapseResult* results = malloc(sizeof(NodeCollapseResult) * root.data.NumChildren);
	uint32_t newNum = 0;
	for (int i = 0; i < root.data.NumChildren; i++) {
		SceneNode child = scene->scene_nodes[scene->node_indices[root.data.childrenIndex + i]];
		results[i] = collapse_node(scene, root.data.transform, &child);
		newNum += results[i].numChildren;
	}
	SceneNode* newNodes = malloc(sizeof(SceneNode) * (newNum + 1));
	int index = 0;
	for (int i = 0;i < root.data.NumChildren;i++) {
		memcpy(&newNodes[index], results[i].children, sizeof(SceneNode) * results[i].numChildren);
		index += results[i].numChildren;
		free(results[i].children);
	}

	SceneNode newRoot = {
		.data.childrenIndex = 0,
		.data.Index = newNum + 1,
		.data.IndexBufferIndex = -1,
		.data.NumTriangles = 0,
		.data.NumChildren = newNum
	};
	newNodes[newNum] = newRoot;
	memcpy(newNodes->data.transform, root.data.transform, sizeof(float) * 4 * 4);
	uint32_t* newIndices = malloc(sizeof(uint32_t) * newNum);
	for (int i = 0; i < newNum; i++) {
		newIndices[i] = i;
	}
	free(scene->scene_nodes);
	free(scene->node_indices);
	scene->scene_nodes = newNodes;
	scene->node_indices = newIndices;
	scene->scene_data.numSceneNodes = newNum + 1;
	scene->scene_data.numNodeIndices = newNum;
}

NodeCollapseResult collapse_node(Scene* scene, float transform[4][4], SceneNode* node) {
	NodeCollapseResult result = { 0 };
	float tr[4][4];
	for (uint32_t r = 0; r != 4; r++) {
		for (uint32_t s = 0; s != 4; s++) {
			tr[r][s] = transform[r][0] * node->data.transform[0][s]
				+ transform[r][1] * node->data.transform[1][s]
				+ transform[r][2] * node->data.transform[2][s]
				+ transform[r][3] * node->data.transform[3][s];
		}
	}
	if (node->data.IndexBufferIndex > -1) { // this node references geometry
		result.numChildren = 1;
		result.children = malloc(sizeof(SceneNode));

		memcpy(result.children[0].data.transform, tr, sizeof(float) * 4 * 4);

		result.children[0].data.NumTriangles = node->data.NumTriangles;
		result.children[0].data.IndexBufferIndex = node->data.IndexBufferIndex;
		result.children[0].data.NumChildren = 0;
		result.children[0].data.childrenIndex = -1;
	}
	for (int i = 0;i < node->data.NumChildren;i++) { // optimizable, less malloc / free
		SceneNode* child = &scene->scene_nodes[scene->node_indices[node->data.childrenIndex + i]];
		NodeCollapseResult childResult = collapse_node(scene, node, child);

		uint32_t newNum = result.numChildren + childResult.numChildren;
		SceneNode* newBuff = malloc(sizeof(SceneNode) * newNum);

		memcpy(newBuff, result.children, sizeof(SceneNode) * result.numChildren);
		memcpy(&newBuff[result.numChildren], childResult.children, sizeof(SceneNode) * childResult.numChildren);
		free(result.children);
		free(childResult.children);
		result.children = newBuff;
		result.numChildren = newNum;
	}
	return result;
}
