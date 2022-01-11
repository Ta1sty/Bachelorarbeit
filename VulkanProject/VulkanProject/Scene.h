#pragma once
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

// gets the index of the i-th child of this node
#define GET_CHILD_IDX(scene, node, i) uint32_t childIdx = ##scene->node_indices[##node->ChildrenIndex + ##i]

// gets a pointer to the i-th child of this node
#define GET_CHILD(scene, node, i) SceneNode* child = &##scene->scene_nodes[##scene->node_indices[##node->ChildrenIndex + ##i]]

// gets the root node of the scene
#define GET_ROOT(scene) SceneNode* root = &##scene->scene_nodes[##scene->scene_data.rootSceneNode]

// this is subject to change
typedef struct viewData {
	float pos[3];
	float pad1;
	float dir[3];
	float pad2;
} ViewData;
// this stays constant (for now)

typedef struct accelerationStructure
{
	VkAccelerationStructureKHR structure;
	VkBuffer buffer;
	VkDeviceMemory memory;
} AccelerationStructure;

typedef struct nodeStructures {
	AccelerationStructure tlas;
	AccelerationStructure blas;
} NodeStructures;

typedef struct sceneNode { // 16 byte alignment (4 floats)
	float object_to_world[4][4];	// 0	64
	float world_to_object[4][4];	// 0	128
	float AABB_min[4];				// 0	144
	float AABB_max[4];				// 0	160
	int32_t IndexBufferIndex;		// 4	164
	int32_t NumTriangles;			// 8	168
	int32_t NumChildren;			// 12	172
	int32_t ChildrenIndex;			// 0	176
	int32_t Index;					// 4	180
	int32_t Level;					// 8	184
	uint32_t NumEven;				// 12	188
	uint32_t NumOdd;				// 0	192
	int32_t TlasNumber;				// 4	196
	float pad1;						// 8	200	// worst case is that we need 2 dummies, for the case that node is even, 
	float pad2;						// 12	204	// references even children and geometry
	float pad3;						// 0	208
} SceneNode;

typedef struct vertex // ALWAYS KEEP THIS PADDED
{
	float position[3];			//0 - 48%16 = 0
	float pad1;					//12
	float normal[3];			//16
	float pad2;					//28
	float tex_x;				//32
	float tex_y;				//36
	int32_t materialIndex;		//40
	float pad3;					//44
} Vertex; // 48 bytes

#define LIGHT_OFF 0 // light is off
#define LIGHT_ON 1 // light is on
#define LIGHT_TYPE_POINT_LIGHT 2 // light is a point light
#define LIGHT_TYPE_DIRECTIONAL_LIGHT 4 // light is a directional light
#define LIGHT_IGNORE_MAX_DISTANCE = 64; // this light is respected regardless of distance
#define LIGHT_USE_MIN_DST = 128; // this light in only respected if there is no occlusion for the minDst range
typedef struct light // 64 bytes
{
	float position[3];
	uint32_t type; // 0th bit = on/off, 1st bit = Point light, 2nd bit = Directional Light
	float intensity[3]; // the intensity for each 
	float maxDst; // the maximum distance this light is still respected
	float quadratic[3]; // the quadratic components [0] + [1]d + [2]d^2
	float radius; // the radius of the light source, everything in it is white.
					// can be negative to make the area around the light less bright
	float direction[3]; // the direction in case this light is directional
	float minDst;	// if this light is directional, the light counts as occluded
					// if there is any object along the fragPos + minDst*direction line
} Light;

typedef struct sceneData
{
	uint32_t numVertices;
	uint32_t numTriangles;
	uint32_t numSceneNodes;
	uint32_t numNodeIndices;
	uint32_t numLights;
	uint32_t rootSceneNode;
} SceneData;

typedef struct renderSettings {
	float fov; // Field of view [0,90)
	uint32_t textures;
	uint32_t ambient;
	uint32_t diffuse;
	uint32_t specular;
	uint32_t shadows;
	uint32_t maxDepth;
	uint32_t debug; // if this is disabled the image is rendered normally
	int32_t colorSensitivity;
	uint32_t displayUV; // displays the triangle UV coordinates
	uint32_t displayTex; // displays the triangle Texture Coordinates
	uint32_t displayTriangles; // displays the Borders of triangles
	uint32_t displayTriangleIdx; // displays the triangleIndex
	uint32_t displayMaterialIdx; // displays the materialIndex
	uint32_t displayTextureIdx; // displays the textureIndex
	uint32_t displayLights; // displays the light sources as Spheres
	uint32_t displayIntersectionT; // displays the intersection T with HSV encoding
	uint32_t displayAABBs; // displays the AABBs
	uint32_t displayTraversalDepth; // displays the maximum Depth the traversal took
	uint32_t displayTraversalCount; // displays the amount of times the loop ran and executed a query (skipped due to hight T is not counted)
	uint32_t displayQueryCount; // displays the total number of rayqueries that were used for this
	uint32_t displayTLASNumber;
} RenderSettings;

typedef struct camera
{
	float pos[3];
	float rotation_x;
	float rotation_y;
	RenderSettings settings;
} Camera;

typedef struct frameData {
	float view_to_world[4][4];
	uint32_t width;
	uint32_t height;
	RenderSettings settings;
} FrameData;

typedef struct material
{
	float k_a;
	float k_d;
	float k_s;
	// TODO Phong exponent
	int32_t texture_index;
} Material;

typedef struct texture
{
	uint32_t index;
	uint32_t image_size;
	uint32_t image_width;
	uint32_t image_height;
	uint32_t* pixel_data;
	VkImage texture_image;
	VkImageView texture_image_view;
	VkDeviceMemory texture_image_memory;
} Texture;

typedef struct texture_data
{
	uint32_t num_materials;
	Material* materials;
	uint32_t num_textures;
	Texture* textures;
} TextureData;

typedef struct scene
{
	VkSampler sampler;
	Camera camera;

	SceneData scene_data;

	TextureData texture_data;

	Vertex* vertices;
	uint32_t* indices;

	SceneNode* scene_nodes;
	uint32_t* node_indices;

	NodeStructures* acceleration_structures; // 1-1 with sceneNodes

	Light* lights;

	uint32_t numTLAS;
	VkAccelerationStructureKHR* TLASs;
} Scene;

typedef struct flatNodeResult
{
	uint32_t num_vertices;
	Vertex* vertices;
} FlatNodeResult;

typedef struct {
	uint32_t numChildren;
	SceneNode* children;
} NodeCollapseResult;

typedef struct sceneSelection
{
	uint32_t numScenes;
	char** availableScenes;

	int currentScene;
	int nextScene;
} SceneSelection;

void init_scene(Scene* scene);
void load_scene(Scene* scene, char* path);
void load_textures(TextureData* data, FILE* file);
void load_texture(Texture* texture, FILE* file);
void flatten_scene(Scene* scene);
void collapse_parent_nodes(Scene* scene);
FlatNodeResult flatten_node(Scene* scene, float parentTransform[4][4], SceneNode* node);
NodeCollapseResult collapse_node(Scene* scene, float transform[4][4], SceneNode* node);
void destroy_scene(Scene* scene);
