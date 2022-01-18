struct Vertex {
	vec3 position; // 0 - 16
	vec3 normal;   // 16 - 16
	vec2 tex_coord;// 32 - 8
	int material_index;   // 40 - 4 - the index of the material to use
};
struct Material {
	float k_a;
	float k_d;
	float k_s;
	int texture_index;
};
struct SceneNode {
	mat4 object_to_world;		// 0
	mat4 world_to_object;		// 0
	vec4 AABB_min;				// 0
	vec4 AABB_max;				// 0
	int IndexBuferIndex;		// 4
	int NumTriangles;			// 8
	int NumChildren;			// 12
	int childrenIndex;			// 0
	int Index;					// 4
	int level;					// 8
	uint numEven;				// 12
	uint numOdd;				// 0
	int tlasNumber;				// 4
	float pad3;					// 8
	float pad4;					// 12
	float pad5;					// 0
};

#define LIGHT_ON 1
#define LIGHT_TYPE_POINT 2
#define LIGHT_TYPE_DIRECTIONAL 4
#define LIGHT_IGNORE_MAX_DISTANCE 64
#define LIGHT_USE_MIN_DST 128
struct Light {
	vec3 position;
	uint type;
	vec3 intensity;
	float maxDst;
	vec3 quadratic;
	float radius;
	vec3 direction;
	float minDst;
};

#define SCENE_DATA_BINDING 0
#define VERTEX_BUFFER_BINDING 1
#define INDEX_BUFFER_BINDING 2
#define MATERIAL_BUFFER_BINDING 3
#define LIGHT_BUFFER_BINDING 4
#define NODE_BUFFER_BINDING 5
#define NODE_CHILDREN_BINDING 6
#define SAMPLER_BINDING 7
#define TEXTURE_BINDING 8
#define FRAME_DATA_BINDING 9
#define TLAS_BINDING 10
#define TRACE_BINDING 11

layout(binding = SCENE_DATA_BINDING, set = 0) uniform SceneData{
	uint numVertices;
	uint numTriangles;
	uint numSceneNodes;
	uint numNodeIndices;
	uint numLights;
	uint rootSceneNode;
};

layout(binding = VERTEX_BUFFER_BINDING, set = 0) buffer VertexBuffer { Vertex[] vertices; };
layout(binding = INDEX_BUFFER_BINDING, set = 0) buffer IndexBuffer { int[] indices; };
layout(binding = MATERIAL_BUFFER_BINDING, set = 0) buffer MaterialBuffer { Material[] materials; };
layout(binding = LIGHT_BUFFER_BINDING, set = 0) buffer LightBuffer { Light[] lights; };
layout(binding = NODE_BUFFER_BINDING, set = 0, row_major) buffer NodeBuffer { SceneNode[] nodes; }; // the array of sceneNodes
layout(binding = NODE_CHILDREN_BINDING, set = 0) buffer ChildBuffer { uint[] childIndices; }; // the index array for node children

layout(binding = SAMPLER_BINDING, set = 1) uniform sampler samp;
layout(binding = TEXTURE_BINDING, set = 1) uniform texture2D textures[];

layout(binding = FRAME_DATA_BINDING, set = 2) uniform FrameData{
	mat4 view_to_world;
	uint width;
	uint height;
	// Render settings struct
	float fov; // Field of view [0,90)
	uint renderTextures;
	uint renderAmbient;
	uint renderDiffuse;
	uint renderSpecular;
	uint renderShadows;
	uint rayMaxDepth;
	uint debug; // if this is disabled the image is rendered normally
	int colorSensitivity;
	uint displayUV; // displays the triangle UV coordinates
	uint displayTex; // displays the triangle Texture Coordinates
	uint displayTriangles; // displays the Borders of triangles
	uint displayTriangleIdx; // displays the TriangleIdx
	uint displayMaterialIdx; // displays the materialIndex
	uint displayTextureIdx; // displays the textureIndex
	uint displayLights; // displays the light sources as Spheres
	uint displayIntersectionT; // displays the intersection T with HSV encoding
	uint displayAABBs; // displays the AABBs
	uint displayTraversalDepth; // displays the maximum Depth the traversal took
	uint displayTraversalCount; // displays the amount of times the loop ran and executed a query (skipped due to hight T is not counted)
	uint displayQueryCount; // displays the total number of rayqueries that were used for this
	uint displayTLASNumber; // displays the tlas number of the first triangle intersection

		// QUERY TRACE
	uint displayQueryTrace;
	uint displayByLevel;
	int selectedLevel;
	uint recordQueryTrace; // if set to one records the querys
	uint pixelX;			// for the pixel with this X
	uint pixelY;			// and this Y coordinate
	uint traceMax;			// max amount (buffer size)
};