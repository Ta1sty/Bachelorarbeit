#version 450
layout(binding = 0, set = 0) uniform sceneData
{
	int numSpheres;
} SceneData;

layout(binding = 1, set = 1) uniform UniformBufferObject {
    vec4 color;
} ubo;



struct Sphere
{
	float pos[3];
	float r;
};



/*layout(binding = 2) uniform sphereBuffer
{
	Sphere[] spheres;
} SphereBuffer;*/


layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
if(SceneData.numSpheres == 2) {
    outColor = vec4(0,0,1, 1.0);
}
 else {
 outColor = vec4(ubo.color.xyz, 1.0);
 }
}