#pragma once
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

// gets the index of the i-th child of this node
#define GET_CHILD_IDX(scene, node, i) uint32_t childIdx = ##scene->node_indices[##node->data.childrenIndex + ##i]

// gets a pointer to the i-th child of this node
#define GET_CHILD(scene, node, i) SceneNode* child = &##scene->scene_nodes[##scene->node_indices[##node->data.childrenIndex + ##i]]

// gets the root node of the scene
#define GET_ROOT(scene) SceneNode* root = &##scene->scene_nodes[##scene->scene_data.numSceneNodes - 1]

// this is subject to change
typedef struct viewData {
	float pos[3];
	float pad1;
	float dir[3];
	float pad2;
} ViewData;
// this stays constant (for now)

typedef struct tlas {
	VkAccelerationStructureKHR structure;
	VkBuffer buffer;
	VkDeviceMemory memory;
	float min[3]; // min Vector for the AABB
	float max[3]; // max Vector for the AABB
} TLAS;
typedef struct blas {
	VkAccelerationStructureKHR structure;
	VkBuffer buffer;
	VkDeviceMemory memory;
	float min[3]; // min Vector for the AABB
	float max[3]; // max Vector for the AABB
} BLAS;

typedef struct sceneNodeData
{
	float transform[4][4]; // transform first cause of alignment
	int32_t IndexBufferIndex; // if this nodes directly references geometry this is >-1					4
	int32_t NumTriangles;	// the amount of triangles in the mesh,ie the range from					8
							//[IndexBufferIndex, IndexBufferIndex + 3 * NumTriangles)					
	int32_t NumChildren;	// the number of children this node references								12
	int32_t childrenIndex; // points to an index array that in turn refers back to the sceneNode index	16
	int32_t Index;			// the own index															20
} SceneNodeData;

typedef struct sceneNode
{
	SceneNodeData data;
	int32_t Level;			// the depth of this node in the scene graph, this is always 1 lower than the lowest parent
	uint32_t numEven;	// > 0 if this node references children with an even level
	uint32_t numOdd;    // > 0 if this node references children with an odd  level
	uint32_t tlas_number;
	TLAS tlas; // the TLAS for this SceneNode, it is set if this node has an even level
	BLAS blas; // the BLAS for this SceneNode, it is set if this node has an odd level or if this node has an even level but it contains geometry
} SceneNode;

typedef struct shaderSceneNode {
	int32_t IndexBuferIndex;
	int32_t NumTriangles;
	int32_t NumChildren;
	int32_t childrenIndex;
	int32_t Index;
	int32_t level;
	uint32_t numEven;
	uint32_t numOdd;
	uint32_t tlasNumber;
} ShaderSceneNode;

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
#define LIGHT_DISTANCE_IGNORE 8 // the intensity remains unchanged by distance
#define LIGHT_DISTANCE_LINEAR 16 // the intensity falls linearly
#define LIGHT_DISTANCE_QUADRATIC 32 // the intensity falls quadraticly
#define LIGHT_IGNORE_MAX_DISTANCE = 64;
#define LIGHT_USE_MIN_DST = 128;
typedef struct light // 24 byte
{
	float position[3];
	uint32_t type; // 0th bit = on/off, 1st bit = Point light, 2nd bit = Directional Light
	float intensity[3]; // the intensity for each 
	float maxDst; // the maximum distance this light is still respected
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
	float fov;
	uint32_t displayUV;
	uint32_t displayTex;
	uint32_t displayTriangles;
	uint32_t displayLights;
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

void init_scene(Scene* scene);
void load_scene(Scene* scene, char** path);
void load_textures(TextureData* data, FILE* file);
void load_texture(Texture* texture, FILE* file);
void flatten_scene(Scene* scene);
void collapse_parent_nodes(Scene* scene);
FlatNodeResult flatten_node(Scene* scene, float parentTransform[4][4], SceneNode* node);
NodeCollapseResult collapse_node(Scene* scene, float transform[4][4], SceneNode* node);
void destroy_scene(Scene* scene);
