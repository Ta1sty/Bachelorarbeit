#version 450

struct Vertex{
	vec3 position; // 0 - 16
	vec3 normal;   // 16 - 16
	vec2 tex_coord;// 32 - 8
	uint tex_index;   // 40 - 4 - the index of the sampler to use
};
layout(binding = 0, set = 0) uniform SceneData{
	uint numVertices;
	uint numTriangles;
	uint numSceneNodes;
	uint numNodeIndices;
};
layout(binding = 1, set = 0) buffer VertexBuffer { Vertex[] vertices; } vertexBuffer;
layout(binding = 2, set = 0) buffer IndexBuffer { int[] indices; } indexBuffer;
layout(binding = 3, set = 1) uniform sampler samp;
layout(binding = 4, set = 1) uniform texture2D textures[];

layout(binding = 5, set = 2) uniform FrameData {
	mat4 view_to_world;
	uint width;
	uint height;
	float fov;
} frame;

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

const float t_min = 1.0e-4f;
const float EPSILON = 0.0000001;

bool rayTriangleIntersect(vec3 rayOrigin, vec3 rayDirection, out vec3 tuv, vec3 v0, vec3 v1, vec3 v2) { 
    vec3 edge1 = v1 - v0;
	vec3 edge2 = v2 - v0;
    vec3 h = cross(rayDirection, edge2);
    float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;    // This ray is parallel to this triangle.
    float f = 1.0f/a;
    vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;
    vec3 q = cross(s, edge1);
    float v = f * dot(rayDirection, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * dot(edge2, q);
    if (t > t_min) // ray intersection
    {
		tuv = vec3(t,u,v);
        vec3 outIntersectionPoint = rayOrigin + rayDirection * t;
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return false;
} 

bool vertexIntersect(vec3 rayOrigin, vec3 rayDir, vec3 postion){
	float t;
	float epsilon = 0.1f;
	vec3 v = rayOrigin - postion;
	float a = rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z;
	float b = 2 * rayDir.x * v.x + 2 * rayDir.y * v.y + 2 * rayDir.z * v.z;
	float c = v.x * v.x + v.y * v.y + v.z * v.z - epsilon * epsilon;
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



bool ray_trace_loop(vec3 rayOrigin, vec3 rayDirection, float t_max, out vec3 tuv, out int triangle_index) {
	vec3 tuv_next;
	tuv.x = t_max;
	triangle_index = -1;
	for(int i = 0;i<numTriangles;i++){
		Vertex vert_v0 = vertexBuffer.vertices[indexBuffer.indices[i * 3]];
		Vertex vert_v1 = vertexBuffer.vertices[indexBuffer.indices[i * 3 + 1]];
		Vertex vert_v2 = vertexBuffer.vertices[indexBuffer.indices[i * 3 + 2]];

		vec3 v0 = vert_v0.position;
		vec3 v2 = vert_v1.position;
		vec3 v1 = vert_v2.position;
		if(rayTriangleIntersect(rayOrigin, rayDirection, tuv_next, v0, v1, v2) ){
			if(tuv_next.x < tuv.x) {
				tuv = tuv_next;
				triangle_index = i;
			}
		}
	}
	if(triangle_index == -1){
		return false;
	}
	return true;
}

void shadeFragment(vec3 P, vec3 N, vec3 V) {
	vec3 k_d = vec3(0.8,0.8,0.8);
	vec3 k_s = vec3(0.8,0.8,0.8);
	float n = 8;
	float i = 5;
	vec3 lightIntenstity = vec3(i,i,i);
	vec3 lightWorldPos = vec3(0.5f,2,-1.5f);
	N = normalize(N);
	vec3 R = normalize(reflect(V, N));
	vec3 color = vec3(0.0);
	vec3 L = normalize(lightWorldPos-P);
	vec3 kd = k_d * max(0,dot(L,N));
	vec3 ks;
	vec3 tuv;
	float t; int index;
	if(!ray_trace_loop(P, L, length(lightWorldPos-P), tuv, index)){
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
	float x = xy.x;
	float y = xy.y;

	float tex_x = x/frame.width;
	float tex_y = y/frame.height;

	float flt_height = frame.height;
	float flt_width = frame.width;
	vec4 origin_view_space = vec4(0,0,0,1);
	vec4 origin_world_space = frame.view_to_world * origin_view_space;

	float z = flt_height/tan(frame.fov);
	vec4 direction_view_space = vec4(normalize(vec3((x - flt_width/2.f),  flt_height/2.f - y, -z)) , 0);
	vec4 direction_world_space = frame.view_to_world * direction_view_space;

	vec3 rayOrigin = origin_world_space.xyz;
	rayOrigin.y = rayOrigin.y+1;
	vec3 rayDirection = direction_world_space.xyz;

	int triangle_index = -1;


	float t_max = 300;
	vec3 tuv;
	if(ray_trace_loop(rayOrigin, rayDirection, t_max, tuv, triangle_index)){
		Vertex vert_v0 = vertexBuffer.vertices[indexBuffer.indices[triangle_index * 3]];
		outColor = vec4(0,0,0,1);
		return;
		vec3 P = rayOrigin + tuv.x * rayDirection;
		vec3 N = vert_v0.normal;
		shadeFragment(P, N, rayDirection);
	}
	else {
		
		for(int i = 0;i<1;i++){
			if(vertexIntersect(rayOrigin, rayDirection, vec3(1,0,0))){
				outColor = vec4(1,0,0,1);
				return;
			}
			if(vertexIntersect(rayOrigin, rayDirection, vec3(0,1,0))){
				outColor = vec4(0,1,0,1);
				return;
			}
			if(vertexIntersect(rayOrigin, rayDirection, vec3(0,0,-1))){
				outColor = vec4(0,0,1,1);
				return;
			}
		}
		outColor = vec4(3, 215, 252, 255) /255;
	}
	return;

	outColor = texture(sampler2D(textures[1], samp), vec2(tex_x, tex_y));
	//outColor = texture(samp, vec2(tex_x, tex_y));
	return;
}