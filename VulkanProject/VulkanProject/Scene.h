#pragma once
#include <stdint.h>
#include <vulkan/vulkan_core.h>

// this is subject to change
typedef struct viewData {
	float pos[3];
	float pad1;
	float dir[3];
	float pad2;
} ViewData;
// this stays constant (for now)

typedef struct sceneNode
{
	float transform[4][4]; // transform first cause of alignment
	int32_t IndexBufferIndex; // if this nodes directly references geometry this is >-1					4
	int32_t NumTriangles;	// the amount of triangles in the mesh,ie the range from					8
							//[IndexBufferIndex, IndexBufferIndex + 3 * NumTriangles)					
	int32_t NumChildren;	// the number of children this node references								12
	int32_t childrenIndex; // points to an index array that in turn refers back to the sceneNode index	16
	int32_t Index;			// the own index															20
} SceneNode;

typedef struct vertex
{
	float position[3];			//0 - 48%16 = 0
	float pad1;					//12
	float normal[3];			//16
	float pad2;					//28
	float tex_x;				//32
	float tex_y;				//36
	uint32_t samplerIndex;		//40
	float pad3;					//44
} Vertex; // 48 bytes

typedef struct sceneData
{
	uint32_t numVertices;
	uint32_t numTriangles;
	uint32_t numSceneNodes;
	uint32_t numNodeIndices;
} SceneData;

typedef struct camera
{
	float pos[3];
	float rotation_x;
	float rotation_y;
} Camera;

typedef struct frameData {
	float view_to_world[4][4];
	uint32_t width;
	uint32_t height;
	float fov;
} FrameData;



typedef struct texture
{
	uint32_t index;
	uint32_t image_size;
	uint32_t image_width;
	uint32_t image_height;
	char* pixel_data;
	VkImage texture_image;
	VkImageView texture_image_view;
	VkDeviceMemory texture_image_memory;
	VkSampler sampler;
} Texture;

typedef struct texture_data
{
	uint32_t num_textures;
	Texture* textures;
} TextureData;

typedef struct scene
{
	Camera camera;
	SceneData scene_data;
	TextureData texture_data;
	Vertex* vertices;
	uint32_t* indices;
	SceneNode* scene_nodes;
	uint32_t* node_indices;
} Scene;

void init_scene(Scene* scene);
int load_scene(Scene* scene, char** path);