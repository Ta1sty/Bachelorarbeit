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
    vec4 cameraPosition;
	vec4 cameraDirection;
	uint width;
	uint height;
	int fov;
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
	vec3 k_d = vec3(0.4,0.4,0.4);
	vec3 k_s = vec3(0.6,0.6,0.6);
	float n = 0.3;
	float i = 3;
	vec3 lightIntenstity = vec3(i,i,i);
	vec3 lightWorldPos = vec3(0.5f,1,0);
	N = normalize(N);
	vec3 R = normalize(reflect(-V, N));
	vec3 color = vec3(0.0);
	vec3 L = normalize(lightWorldPos-P);
	vec3 kd = k_d * max(0,dot(L,N));
	vec3 ks;
	float t; int index;
	if(!ray_trace_loop(P, L, length(P-lightWorldPos), t, index)){
		ks = k_s * pow(max(0,dot(R,L)),n);
	} else {
		ks = vec3(0);
	}
	vec3 light = lightIntenstity/(dot(P - lightWorldPos,P - lightWorldPos));
	color += (kd)*light;
	outColor = vec4(color, 1);
}



void main() {
    float scale = tan(radians(frame.fov * 0.5)); 
	float flt_width = frame.width;
	float flt_hight = frame.height;
	float imageAspectRatio = flt_width / flt_hight;

	ivec2 ij = ivec2(gl_FragCoord.xy);
	
	float x = (2 * (ij.x + 0.5) / flt_width - 1) * imageAspectRatio * scale; 
	float y = (1 - 2 * (ij.y + 0.5) / flt_hight) * scale;
	float z = 1;

	vec3 rayOrigin = vec3(0); 
	vec3 rayDirection = vec3(x, y, 1) - rayOrigin;

	rayDirection = normalize(rayDirection);

	float t_best = 500;

	//for debugging
	bool debug = false;
	if(debug && SphereBuffer.spheres[1].r ==2) {
		outColor = vec4(1,1,1,1);
		return;
	}

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