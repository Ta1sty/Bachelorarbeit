#version 460
#define RAY_TRACE
#extension GL_EXT_nonuniform_qualifier : enable
#ifdef RAY_TRACE
#extension GL_EXT_ray_query : require
#endif

struct Vertex{
	vec3 position; // 0 - 16
	vec3 normal;   // 16 - 16
	vec2 tex_coord;// 32 - 8
	int material_index;   // 40 - 4 - the index of the material to use
};
struct Material{
	float k_a;
	float k_d;
	float k_s;
	int texture_index;
};

layout(binding = 0, set = 0) uniform SceneData{
	uint numVertices;
	uint numTriangles;
	uint numSceneNodes;
	uint numNodeIndices;
};

layout(binding = 1, set = 0) buffer VertexBuffer { Vertex[] vertices; };
layout(binding = 2, set = 0) buffer IndexBuffer { int[] indices; };
layout(binding = 3, set = 0) buffer MaterialBuffer { Material[] materials; };
layout(binding = 4, set = 1) uniform sampler samp;
layout(binding = 5, set = 1) uniform texture2D textures[];

layout(binding = 6, set = 2) uniform FrameData {
	mat4 view_to_world;
	uint width;
	uint height;
	float fov;
	uint displayUV;
	uint displayTex;
} frame;

#ifdef RAY_TRACE
layout(binding = 7, set = 3) uniform accelerationStructureEXT tlas;
#endif

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
#ifdef RAY_TRACE
	tuv = vec3(0);
	float min_t = 1.0e-3f;
	float max_t = t_max;
	rayQueryEXT ray_query;
	rayQueryInitializeEXT(ray_query, tlas, 0, 0xFF, rayOrigin, min_t, rayDirection, max_t);
	while(rayQueryProceedEXT(ray_query)){
        if(rayQueryGetIntersectionTypeEXT(ray_query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) { // triangle intersection
                rayQueryConfirmIntersectionEXT(ray_query); // might want to check for opaque
		}
	}
	if(rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT ) {
		triangle_index = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, true);
		tuv.x = rayQueryGetIntersectionTEXT(ray_query, true);
		vec2 uv = rayQueryGetIntersectionBarycentricsEXT(ray_query, true);
		tuv.y = uv.y;
		tuv.z = uv.x;
		return true;
	}
	return false;
#endif
#ifndef RAY_TRACE
	vec3 tuv_next;
	tuv.x = t_max;
	triangle_index = -1;
	for(int i = 0;i<numTriangles;i++){
		Vertex vert_v0 = vertices[indices[i * 3]];
		Vertex vert_v1 = vertices[indices[i * 3 + 1]];
		Vertex vert_v2 = vertices[indices[i * 3 + 2]];

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
#endif
}

void shadeFragment(vec3 P, vec3 V, vec3 tuv, int triangle) {
	Vertex v0 = vertices[indices[triangle * 3]];
	Vertex v1 = vertices[indices[triangle * 3 + 1]];
	Vertex v2 = vertices[indices[triangle * 3 + 2]];


	float w = 1 - tuv.y - tuv.z;
	float u = tuv.y;
	float v = tuv.z;

	if(frame.displayUV != 0){
		outColor = vec4(u,v,0,1);
		return;
	}

	vec3 N = w * v0.normal + v * v1.normal + u * v2.normal;
	N = normalize(N);
	vec2 tex = w * v0.tex_coord + v * v1.tex_coord + u * v2.tex_coord;

	if(frame.displayTex != 0){
		outColor = vec4(tex.xy,0,1);
		return;
	}

	float ka, kd, ks;
	vec3 color;
	if(v0.material_index < 0){
		ka = 0.3f; kd = 0.4f; ks = 0.3f;
		color = vec3(1,0,1);
	} else {
		Material m = materials[v0.material_index];
		ka = m.k_a;
		kd = m.k_d;
		ks = m.k_s;
		if(m.texture_index < 0)
			color = vec3(1,0,1);
		else
			color = texture(sampler2D(textures[m.texture_index], samp), tex).xyz;
	}

	float n = 1;
	float i = 100;
	vec3 lightIntenstity = vec3(i,i,i);
	vec3 lightWorldPos = vec3(0,10,0);
	vec3 R = normalize(reflect(V, N));
	vec3 L = normalize(lightWorldPos-P);
	vec3 tuv2;
	float t; int index;
	if(!ray_trace_loop(P, L, length(lightWorldPos-P), tuv2, index)){ // is light source visible?
		ks = ks * pow(max(0,dot(R,L)),n);
	} else {
		ks = 0;
		kd = 0;
	}
	vec3 light = lightIntenstity/(dot(P - lightWorldPos,P - lightWorldPos));
	light = (kd+ks)*light;
	outColor = vec4(color * light + color * ka, 1);
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
	vec3 rayDirection = direction_world_space.xyz;

	int triangle_index = -1;

	float t_max = 300;
	vec3 tuv;
	if(ray_trace_loop(rayOrigin, rayDirection, t_max, tuv, triangle_index)){
		vec3 P = rayOrigin + tuv.x * rayDirection;
		shadeFragment(P, rayDirection, tuv, triangle_index);
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
			if(vertexIntersect(rayOrigin, rayDirection, vec3(0,0,0))){
				outColor = vec4(1,1,1,1);
				return;
			}		
		}
		outColor = vec4(3, 215, 252, 255) /255;
	}
	return;
}