#version 450
layout(binding = 0, set = 0) uniform sceneData
{
	int numSpheres;
} SceneData;

struct Sphere
{
float x;
float y;
float z;
float r;
};

layout(binding = 1, set = 0) buffer sphereBuffer {
	Sphere[] spheres;
} SphereBuffer;

layout(binding = 2, set = 1) uniform FrameData {
	mat4 view_to_world;
	uint width;
	uint height;
	float fov;
} frame;

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

const float t_min = 1.0e-3f;

bool sphereIntersect(vec3 rayOrigin, vec3 rayDir, int sphereIndex, out float t){
	Sphere s = SphereBuffer.spheres[sphereIndex];
	vec3 v = rayOrigin - vec3(s.x,s.y,s.z);
	float a = rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z;
	float b = 2 * rayDir.x * v.x + 2 * rayDir.y * v.y + 2 * rayDir.z * v.z;
	float c = v.x * v.x + v.y * v.y + v.z * v.z - s.r * s.r;
	float disciminant = b * b - 4 * a * c;
	if (disciminant < 0) return false;

	if (disciminant == 0) {
		t = -b / (2 * a);
		if (t < t_min) return false;
		return true;
	}
	disciminant = sqrt(disciminant);
	float t1 = (-b - disciminant)/(2*a);
	float t2 = (-b + disciminant)/(2*a);
	//this only works because a is strictly positive
	if (t2 < t_min) return false;
	if (t1 < t_min) {
		t = t2;
		return true;
	}
	t = t1;
	return true;
}
bool ray_trace_loop(vec3 rayOrigin, vec3 rayDirection, float t_max, out float t, out int sphereIndex) {
	float t_max_in = t_max;
	for(int i = 0; i < SceneData.numSpheres; i++){
		t = t_max;
		if(sphereIntersect(rayOrigin, rayDirection, i, t)) {
			if(t<t_max) {
				t_max = t;
				sphereIndex = i;
			}
		}
	}
	if(t_max == t_max_in) return false;
	t = t_max;
	return true;
}

void shadeSphere(vec3 P, vec3 N, vec3 V) {
	vec3 k_d = vec3(0.8,0.2,0.5);
	vec3 k_s = vec3(0.2,0.8,0.5);
	float n = 8;
	float i = 5;
	vec3 lightIntenstity = vec3(i,i,i);
	vec3 lightWorldPos = vec3(0,2,0);
	N = normalize(N);
	vec3 R = normalize(reflect(V, N));
	vec3 color = vec3(0.0);
	vec3 L = normalize(lightWorldPos-P);
	vec3 kd = k_d * max(0,dot(L,N));
	vec3 ks;
	float t; int index;
	if(!ray_trace_loop(P, L, length(lightWorldPos-P), t, index)){
		ks = k_s * pow(max(0,dot(R,L)),n);

	} else {
		ks = vec3(0);
	}
	vec3 light = lightIntenstity/(dot(P - lightWorldPos,P - lightWorldPos));
	color += (ks+kd)*light;
	outColor = vec4(color, 1);
}


void main() {
	// from https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-generating-camera-rays/generating-camera-rays
	ivec2 xy = ivec2(gl_FragCoord.xy);
	int x = xy.x;
	int y = xy.y;

	float flt_height = frame.height;
	float flt_width = frame.width;
	vec4 origin_view_space = vec4(0,0,0,1);
	vec4 origin_world_space = frame.view_to_world * origin_view_space;

	float z = flt_height/tan(frame.fov);
	vec4 direction_view_space = vec4(normalize(vec3((x - flt_width/2.f), y - flt_height/2.f, -z)) , 0);
	vec4 direction_world_space = frame.view_to_world * direction_view_space;

	vec3 rayOrigin = origin_world_space.xyz;
	vec3 rayDirection = direction_world_space.xyz;
	rayDirection.y = -rayDirection.y;
	rayDirection.z = -rayDirection.z;

	float t_best = 500;
	float t_max = 500;
	float t;
	int sphereIndex = -1;
	if(ray_trace_loop(rayOrigin, rayDirection, t_max, t, sphereIndex)){
		if(sphereIndex == 0)
			outColor = vec4(1,0,0,1);
		if(sphereIndex == 1)
			outColor = vec4(0,1,0,1);
		vec3 P = rayOrigin + t * rayDirection;
		Sphere s = SphereBuffer.spheres[sphereIndex];
		vec3 N = normalize(P - vec3(s.x, s.y, s.z));
		shadeSphere(P, N, rayDirection);
	} else {
		outColor = vec4(3, 215, 252, 255) /255;
		//outColor = vec4(abs(x), abs(y) ,1, 1);
	}
}