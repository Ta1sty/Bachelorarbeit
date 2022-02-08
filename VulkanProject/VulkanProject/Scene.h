#pragma once
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

// gets the index of the i-th child of this node
#define GET_CHILD_IDX(scene, node, i) uint32_t childIdx = ##scene->node_indices[##node->ChildrenIndex + ##i]

// gets a pointer to the i-th child of this node
#define GET_CHILD(scene, node, i) SceneNode* child = &##scene->scene_nodes[##scene->node_indices[##node->ChildrenIndex + ##i]]
#define GET_GRANDCHILD(scene, child, i) SceneNode* grandChild = &##scene->scene_nodes[##scene->node_indices[##child->ChildrenIndex + ##i]]

// gets the root node of the scene
#define GET_ROOT(scene) SceneNode* root = &##scene->scene_nodes[##scene->scene_data.rootSceneNode]

#define TRANSFORM(scene, node) &##scene->node_transforms[##node->TransformIndex]
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

typedef struct mat4x3 { // 4 collumns, 3 rows. Row major
	float mat[3][4];
} Mat4x3;

typedef struct sceneNode { // 16 byte alignment (4 floats)
	float AABB_min[3];				// 12	60
	int32_t Index;					// 0	64
	float AABB_max[3];				// 12	76
	int32_t Level;					// 0	80
	int32_t NumTriangles;			// 4	84
	int32_t IndexBufferIndex;		// 8	88
	int32_t NumChildren;			// 12	92
	int32_t ChildrenIndex;			// 0	96
	int32_t TlasNumber;				// 4	100
	VkBool32 IsInstanceList;		// 8	104
	VkBool32 IsLodSelector;			// 12	108
	uint32_t TransformIndex;		// 0	112-48 = 64
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
#define LIGHT_TYPE_SUN 8
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
	uint32_t numTransforms;
	uint32_t numNodeIndices;
	uint32_t numLights;
	uint32_t rootSceneNode;
} SceneData;

typedef struct renderSettings {
	float fov; // Field of view [0,90)
	VkBool32 textures;
	VkBool32 ambient;
	VkBool32 diffuse;
	VkBool32 specular;
	VkBool32 shadows;
	uint32_t maxDepth;
	VkBool32 debug; // if this is disabled the image is rendered normally
	int32_t colorSensitivity;
	// GENERAL INFO
	VkBool32 displayUV; // displays the triangle UV coordinates
	VkBool32 displayTex; // displays the triangle Texture Coordinates
	VkBool32 displayTriangles; // displays the Borders of triangles
	VkBool32 displayTriangleIdx; // displays the triangleIndex
	VkBool32 displayMaterialIdx; // displays the materialIndex
	VkBool32 displayTextureIdx; // displays the textureIndex
	VkBool32 displayLights; // displays the light sources as Spheres
	VkBool32 displayIntersectionT; // displays the intersection T with HSV encoding

	// QUERY INFO
	VkBool32 displayAABBs; // displays the AABBs
	VkBool32 displayListAABBs; // displas the AABBs of instance Lists
	VkBool32 displayTraversalDepth; // displays the maximum Depth the traversal took
	VkBool32 displayTraversalCount; // displays the amount of times the loop ran and executed a query (skipped due to hight T is not counted)
	VkBool32 displayQueryCount; // displays the total number of rayqueries that were used for this
	VkBool32 displayTLASNumber;

	// QUERY TRACE
	VkBool32 displayQueryTrace;
	VkBool32 displayByLevel;
	int32_t selectedLevel;
	VkBool32 recordQueryTrace;
	uint32_t pixelX;
	uint32_t pixelY;
	uint32_t traceMax;
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
	Mat4x3* node_transforms;
	uint32_t* node_indices;

	NodeStructures* acceleration_structures; // 1-1 with sceneNodes

	Light* lights;

	uint32_t numTLAS;
	VkAccelerationStructureKHR* TLASs;
} Scene;

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
void destroy_scene(Scene* scene);
