#pragma once
#include <stdint.h>

// this is subject to change
typedef struct viewData {
	float pos[3];
	float pad1;

	float dir[3];
	float pad2;
} ViewData;
// this stays constant (for now)
typedef struct triangle
{
	float v1[3];
	float v2[3];
	float v3[3];
	float pad3[7];
}Triangle;

typedef struct sceneNode
{
	int32_t IndexBufferIndex; // if this nodes directly references geometry this is >-1					4
	int32_t NumTriangles;	// the amount of triangles in the mesh,ie the range from					8
							//[IndexBufferIndex, IndexBufferIndex + 3 * NumTriangles)					
	int32_t NumChildren;	// the number of children this node references								12
	int32_t childrenIndex; // points to an index array that in turn refers back to the sceneNode index	16
	int32_t Index;			// the own index															20
	float transform[4][4];
} SceneNode;

typedef struct sphere
{
	float x;
	float y;
	float z;
	float r;
} Sphere;

typedef struct sphereData
{
	Sphere* spheres;
} SphereData;

typedef struct sceneData
{
	uint32_t sphere_count;
	SphereData sphere_data;
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