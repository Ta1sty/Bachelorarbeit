#pragma once
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

typedef struct frameData {
	float cameraPos[4];
	float cameraDir[4];
	uint32_t width;
	uint32_t height;
	uint32_t fov;
} FrameData;