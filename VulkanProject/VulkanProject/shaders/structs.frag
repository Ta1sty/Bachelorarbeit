struct Vertex {
	vec3 position; // 0 - 16
	float pad1;
	vec3 normal;   // 16 - 16
	float pad2;
	vec2 tex_coord;// 32 - 8
	int material_index;   // 40 - 4 - the index of the material to use
	float pad3;
};
struct Material {
	float k_a;
	float k_d;
	float k_s;
	int texture_index;
};
struct SceneNode {
	vec3 AABB_min;				// 12
	int Index;					// 0
	vec3 AABB_max;				// 12
	int Level;					// 0
	int NumTriangles;			// 4
	int IndexBuferIndex;		// 8
	int NumChildren;			// 12
	int ChildrenIndex;			// 0
	int TlasNumber;				// 4
	bool IsInstanceList;		// 8
	bool IsLodSelector;			// 12
	uint TransformIndex;		// 0
};

#define LIGHT_ON 1
#define LIGHT_TYPE_POINT 2
#define LIGHT_TYPE_DIRECTIONAL 4
#define LIGHT_TYPE_SUN 8
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
#define TRANSFORM_BUFFER_BINDING 6
#define NODE_CHILDREN_BINDING 7
#define SAMPLER_BINDING 8
#define TEXTURE_BINDING 9
#define FRAME_DATA_BINDING 10
#define TLAS_BINDING 11
#define TRACE_BINDING 12

layout(binding = SCENE_DATA_BINDING, set = 0) uniform SceneData{
	uint numVertices;
	uint numTriangles;
	uint numSceneNodes;
	uint numTransforms;
	uint numNodeIndices;
	uint numLights;
	uint rootSceneNode;
};

layout(binding = VERTEX_BUFFER_BINDING, set = 0) buffer VertexBuffer { Vertex[] vertices; };
layout(binding = INDEX_BUFFER_BINDING, set = 0) buffer IndexBuffer { int[] indices; };
layout(binding = MATERIAL_BUFFER_BINDING, set = 0) buffer MaterialBuffer { Material[] materials; };
layout(binding = LIGHT_BUFFER_BINDING, set = 0) buffer LightBuffer { Light[] lights; };
layout(binding = NODE_BUFFER_BINDING, set = 0, row_major) buffer NodeBuffer { SceneNode[] nodes; }; // the array of sceneNodes
layout(binding = TRANSFORM_BUFFER_BINDING, set = 0, row_major) buffer TransformBuffer { mat4x3[] transforms; }; // the array of node transforms
layout(binding = NODE_CHILDREN_BINDING, set = 0) buffer ChildBuffer { uint[] childIndices; }; // the index array for node children

layout(binding = SAMPLER_BINDING, set = 1) uniform sampler samp;
layout(binding = TEXTURE_BINDING, set = 1) uniform texture2D textures[];

layout(binding = FRAME_DATA_BINDING, set = 2) uniform FrameData{
	mat4 view_to_world;
	uint width;
	uint height;
	// Render settings struct
	float fov; // Field of view [0,90)
	bool renderTextures;
	bool renderAmbient;
	bool renderDiffuse;
	bool renderSpecular;
	bool renderShadows;
	uint rayMaxDepth;
	bool debug; // if this is disabled the image is rendered normally
	int colorSensitivity;
	bool displayUV; // displays the triangle UV coordinates
	bool displayTex; // displays the triangle Texture Coordinates
	bool displayTriangles; // displays the Borders of triangles
	bool displayTriangleIdx; // displays the TriangleIdx
	bool displayMaterialIdx; // displays the materialIndex
	bool displayTextureIdx; // displays the textureIndex
	bool displayLights; // displays the light sources as Spheres
	bool displayIntersectionT; // displays the intersection T with HSV encoding
	bool displayAABBs; // displays the AABBs
	bool displayListAABBs; // displays the AABBs of Instance Lists (Huge FPS drops)
	bool displayTraversalDepth; // displays the maximum Depth the traversal took
	bool displayTraversalCount; // displays the amount of times the loop ran and executed a query (skipped due to hight T is not counted)
	bool displayQueryCount; // displays the total number of rayqueries that were used for this
	bool displayTLASNumber; // displays the tlas number of the first triangle intersection

		// QUERY TRACE
	bool displayQueryTrace;
	bool displayByLevel;
	int selectedLevel;
	bool recordQueryTrace; // if set to one records the querys
	uint pixelX;			// for the pixel with this X
	uint pixelY;			// and this Y coordinate
	uint traceMax;			// max amount (buffer size)
};