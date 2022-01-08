#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#ifdef RAY_TRACE
#extension GL_EXT_ray_query : require
#endif		
#define PI 3.1415926538
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
struct SceneNode{
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
const uint LIGHT_ON = 1;
const uint LIGHT_TYPE_POINT = 2;
const uint LIGHT_TYPE_DIRECTIONAL = 4;
const uint LIGHT_IGNORE_MAX_DISTANCE = 64;
const uint LIGHT_USE_MIN_DST = 128;
struct Light{
	vec3 position;
	uint type;
	vec3 intensity;
	float maxDst;
	vec3 quadratic;
	float radius;
	vec3 direction;
	float minDst;
};

layout(binding = 0, set = 0) uniform SceneData{
	uint numVertices;
	uint numTriangles;
	uint numSceneNodes;
	uint numNodeIndices;
	uint numLights;
	uint rootSceneNode;
};

layout(binding = 1, set = 0) buffer VertexBuffer { Vertex[] vertices; };
layout(binding = 2, set = 0) buffer IndexBuffer { int[] indices; };
layout(binding = 3, set = 0) buffer MaterialBuffer { Material[] materials; };
layout(binding = 4, set = 0) buffer LightBuffer {Light[] lights;};
// Scene nodes
// correleates 1-1 with NodeBuffer, is alone cause of alignment
layout(binding = 5, set = 0, row_major) buffer NodeBuffer { SceneNode[] nodes;}; // the array of sceneNodes
layout(binding = 6, set = 0) buffer ChildBuffer { uint[] childIndices;}; // the index array for node children

layout(binding = 7, set = 1) uniform sampler samp;
layout(binding = 8, set = 1) uniform texture2D textures[];

layout(binding = 9, set = 2) uniform FrameData {
	mat4 view_to_world;
	uint width;
	uint height;
	// Render settings struct
	float fov; // Field of view [0,90)
	uint debug; // if this is disabled the image is rendered normally
	int colorSensitivity;
	uint displayUV; // displays the triangle UV coordinates
	uint displayTex; // displays the triangle Texture Coordinates
	uint displayTriangles; // displays the Borders of triangles
	uint displayLights; // displays the light sources as Spheres
	uint displayIntersectionT; // displays the intersection T with HSV encoding
	uint displayAABBs; // displays the AABBs
	uint displayTraversalDepth; // displays the maximum Depth the traversal took
	uint displayTraversalCount; // displays the amount of times the loop ran and executed a query (skipped due to hight T is not counted)
	uint displayQueryCount; // displays the total number of rayqueries that were used for this
	uint displayTLASNumber; // displays the tlas number of the first triangle intersection
};

#ifdef RAY_TRACE
layout(binding = 10, set = 3) uniform accelerationStructureEXT[] tlas;
#endif

layout(location = 0) in vec3 fragColor;
vec4 debugColor; // if last is 0 it was never set, in that case use outColor
layout(location = 0) out vec4 outColor;

//https://gist.github.com/983/e170a24ae8eba2cd174f
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

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
bool vertexIntersect(vec3 rayOrigin, vec3 rayDir, vec3 postion, out float t){
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

void getHitPayload(int triangle, vec3 tuv, out vec4 color, out vec3 N, out Material material){
	Vertex v0 = vertices[indices[triangle * 3]];
	Vertex v1 = vertices[indices[triangle * 3 + 1]];
	Vertex v2 = vertices[indices[triangle * 3 + 2]];

	float w = 1 - tuv.y - tuv.z;
	float u = tuv.y;
	float v = tuv.z;
	
	if(displayUV != 0){ // debug
		debugColor = vec4(u,v,0,1);
	}
	// compute interpolated Normal and Tex - important this is in object space -> need to transform either 
	N = w * v0.normal + v * v1.normal + u * v2.normal;
	N = normalize(N);
	vec2 tex = w * v0.tex_coord + v * v1.tex_coord + u * v2.tex_coord;

	if(displayTex != 0){	// debug
		debugColor = vec4(tex.xy,0,1);
	}

	// get material and texture properties, if there are not set use default values
	if(v0.material_index < 0){
		material.k_a = 0.15f;
		material.k_d = 0.5f;
		material.k_s = 0.35f;
		material.texture_index = -1;
		color = vec4(0.2,0.4,0.8,1);
	} else {
		material = materials[v0.material_index];
		if(material.texture_index < 0)
			color = vec4(0.2,0.4,0.8,1);
		else
			color = texture(sampler2D(textures[material.texture_index], samp), tex);
	}
}

// ONLY modify these when in the instanceShader or the rayTrace Loop
struct TraversalPayload{
	vec3 transformed_origin; // ray origin in object space
	float t; // the t for which this aabb was intersected
	vec3 transformed_direction; // the ray origin in object space
	uint node_idx; // the SceneNode to which this traversalNode belongs
};
// use a stack because:
// keep the list as short as possible, I.E. use a depth first search
const int BUFFER_SIZE = 14;
int stackSize = 0;
TraversalPayload traversalBuffer[BUFFER_SIZE];

// Problem, want to reconstruct the tranform path for the Normal and potentially other applications
// Idea, have 2 arrays int[MAX_DEPTH] commited, candidate
// if triangle is accepted by traversal, copy contents of candidate to commited. This array then contains the levels used.
// in case there is a skip from exapmle level 3->6, we set 4,5 to -1 so we know to ignore them
// for traversal proceed as follows. once node is popped write node.index to candidate[node.level]
// if a path fails without any 

// for a given rayQuery this method returns a ray and a tlasNumber
#ifdef RAY_TRACE
void instanceHit(rayQueryEXT ray_query, TraversalPayload load, SceneNode node) {
	float t = rayQueryGetIntersectionTEXT(ray_query, false);
	//if(t>best_t) return; // we already found a triangle outside this aabb before this was even hit we dont need to add this instance
	// this can never happen since best_t is our t_max for this query
	uint iIdx = rayQueryGetIntersectionInstanceIdEXT(ray_query, false);
	uint cIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, false);
	uint pIdx = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, false);
	SceneNode next;
	if(node.numEven>0 && iIdx == 0){ // only do the even case for instance 0 and if there are even nodes, otherwise any aabb is ODD
		// TODO this case is avoided from now on, since the compiler eliminates every odd-odd and even-even pair
		// handle even child
		next = nodes[childIndices[node.childrenIndex+pIdx]];
	} else {
		// handle odd child
		SceneNode directChild = nodes[cIdx]; // use the custom index to take a shortcut
		next = nodes[childIndices[directChild.childrenIndex + pIdx]];
	}

	// we can now do LOD or whatever we feel like doing


	vec3 origin = rayQueryGetIntersectionObjectRayOriginEXT(ray_query, false);
	vec3 direction = rayQueryGetIntersectionObjectRayDirectionEXT(ray_query, false);

	// here the shader adds the next payloads
	TraversalPayload nextLoad;
	nextLoad.transformed_origin = (next.world_to_object * vec4(origin,1)).xyz;
	nextLoad.transformed_direction = (next.world_to_object * vec4(direction,0)).xyz;
	nextLoad.node_idx = next.Index;
	nextLoad.t = t;
	traversalBuffer[stackSize] = nextLoad;
	stackSize++;
}

void triangleHit(rayQueryEXT ray_query, SceneNode node){
	float t = rayQueryGetIntersectionTEXT(ray_query, false);
	vec2 uv = rayQueryGetIntersectionBarycentricsEXT(ray_query, false);
	uint cIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, false);
	SceneNode blasChild = nodes[cIdx];
	int triangle = blasChild.IndexBuferIndex/3 + rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, false);
	Vertex v0 = vertices[indices[triangle * 3]];
	Vertex v1 = vertices[indices[triangle * 3 + 1]];
	Vertex v2 = vertices[indices[triangle * 3 + 2]];

	vec3 tuv;
	tuv.x = t;
	tuv.y = uv.y;
	tuv.z = uv.x;
	vec3 N;
	vec4 color;
	Material material;
	getHitPayload(triangle, tuv, color, N, material);
	if(color[3] > 0)
		rayQueryConfirmIntersectionEXT(ray_query);
}

#endif


int maxDepth = 0;
uint numTraversals = 0;
uint queryCount = 0;
bool ray_trace_loop(vec3 rayOrigin, vec3 rayDirection, float t_max, uint root, out vec3 tuv, out int triangle_index) {
#ifdef RAY_TRACE

	numTraversals = 0;

	tuv = vec3(0);

	float min_t = 1.0e-3f;
	float best_t = t_max;

	TraversalPayload start; // start at root node
	start.transformed_origin = rayOrigin;
	start.transformed_direction = rayDirection;
	start.node_idx = root;

	traversalBuffer[0] = start;

	triangle_index = -1;
	stackSize = 1;

	uint triangleTLAS = -1;

	while(stackSize>0){
		TraversalPayload load = traversalBuffer[stackSize-1];
		stackSize--; // remove last element

		if(load.t > best_t) continue;// there was already a closer hit, we can skip this one

		SceneNode node = nodes[load.node_idx]; // retrieve scene Node
		maxDepth = max(node.level, maxDepth); // not 100% correct but it gives a vague idea
		uint tlasNumber = node.tlasNumber;
        vec3 query_origin = load.transformed_origin;
        vec3 query_direction = load.transformed_direction;

		rayQueryEXT ray_query;
		rayQueryInitializeEXT(ray_query, tlas[tlasNumber], 0, 0xFF, 
			query_origin, min_t, 
			query_direction, best_t);
		queryCount++;
		numTraversals++;
		while(rayQueryProceedEXT(ray_query)){
			uint type = rayQueryGetIntersectionTypeEXT(ray_query, false);
			switch(type){
				case gl_RayQueryCandidateIntersectionTriangleEXT:
					// might want to check for opaque
					triangleHit(ray_query, node);
					break;
				case gl_RayQueryCandidateIntersectionAABBEXT:
					// we do not want to generate intersections, since AABB hit does not guarantee a hit in the traversal
					// instead call instanceShader and add the new parameters to the traversalList
					instanceHit(ray_query, load, node);
					break;
				default: break;
			}
		}

		uint commitedType = rayQueryGetIntersectionTypeEXT(ray_query, true);
		if(commitedType == gl_RayQueryCommittedIntersectionTriangleEXT ) {
			if(debug != 0 && displayTriangles != 0){
				vec2 uv = rayQueryGetIntersectionBarycentricsEXT(ray_query, true);
				if(uv.x+uv.y>0.99f || uv.x < 0.01f || uv.y < 0.01f) {
					triangle_index = 0;
					return true;
				} else {
					continue;
				}
			}
			float t = rayQueryGetIntersectionTEXT(ray_query, true);
			if(t<best_t){
				best_t = t;

				uint cIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, true);
				SceneNode blasChild = nodes[cIdx];
				triangle_index = blasChild.IndexBuferIndex/3 + rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, true);


				tuv.x = t;
				vec2 uv = rayQueryGetIntersectionBarycentricsEXT(ray_query, true);
				tuv.y = uv.y;
				tuv.z = uv.x;

				triangleTLAS = tlasNumber;
			}
			// triangle_index = -1;
		}
	}
	if(displayTLASNumber != 0 && debugColor[3] == 0){	// debug
		debugColor = vec4(hsv2rgb(vec3(min(triangleTLAS * 1f/colorSensitivity,0.66f),1,1)),1);
	}
	if(triangle_index >= 0) return true;
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
vec4 shadeFragment(vec3 P, vec3 V, vec4 color, vec3 N, Material material) {
	float n = 1; // todo phong exponent
	// calculate lighting for each light source

	vec3 sum = vec3(0);
	for(int i = 0;i<numLights;i++){
		Light light = lights[i];

		if((light.type & LIGHT_ON) == 0){ // is the light on?
			continue;
		}
		vec3 tuvShadow;
		int index;

		vec3 L = light.position - P;
		float l_dst = length(L);
		if(l_dst > light.maxDst && (light.type & LIGHT_IGNORE_MAX_DISTANCE) == 0){ // is the light near enough to even matter
			continue;
		}
		float specular;
		float diffuse = material.k_d;
		vec3 R = normalize(reflect(V, N));
		if((light.type & LIGHT_TYPE_POINT) != 0){ // point light, check if it is visible and then compute KS via L vector
			vec3 LN = normalize(L);
			if(ray_trace_loop(P, LN, l_dst,rootSceneNode, tuvShadow, index)){ // is light source visible, shoot ray to lPos?
				continue;
			}
			specular = material.k_s * pow(max(0,dot(R,LN)),n);
		} else if((light.type & LIGHT_TYPE_DIRECTIONAL) != 0) { // directional light, check if it is visible 
			if((light.type & LIGHT_USE_MIN_DST) != 0){ // shoot ray over minDst, if nothing is hit lighting is enabled
				if(ray_trace_loop(P, normalize(-light.direction), light.minDst,rootSceneNode, tuvShadow, index)){ // shoot shadow ray into the light source
					continue;
				}
				specular = material.k_s * pow(max(0,dot(R,-light.direction)),n);
			} else { // directional light with fixed position, this is probably not correct
				if(ray_trace_loop(P, normalize(-light.direction), l_dst,rootSceneNode, tuvShadow, index)){
					continue;
				}
				specular = material.k_s * pow(max(0,dot(R,-light.direction)),n);
			}
		}
		// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff
		float d = l_dst - light.radius;
		if(d<0)
			sum+=vec3(1);
		else {
			float l_mult = light.quadratic.x + light.quadratic.y / d + light.quadratic.z / (d*d);
			sum += (specular+diffuse) * l_mult * light.intensity * color.xyz;
		}
	}
	material.k_a = 0;
	sum += material.k_a * color.xyz;
	return vec4(sum.xyz, color[3]);
}

void generatePixelRay(out vec3 rayOrigin, out vec3 rayDirection){
	ivec2 xy = ivec2(gl_FragCoord.xy);
	float x = xy.x;
	float y = xy.y;

	float flt_height = height;
	float flt_width = width;
	vec4 origin_view_space = vec4(0,0,0,1);
	vec4 origin_world_space = view_to_world * origin_view_space;
	float z = flt_height/tan(PI / 180 * fov);
	vec4 direction_view_space = vec4(normalize(vec3((x - flt_width/2.f),  flt_height/2.f - y, -z)) , 0);
	vec4 direction_world_space = view_to_world * direction_view_space;

	rayOrigin = origin_world_space.xyz;
	rayDirection = direction_world_space.xyz;
}
const int MAX_DEPTH = 5;
const float MAX_T = 300;
vec4 rayTrace(vec3 rayOrigin, vec3 rayDirection, out float t) {
	vec4 color = vec4(0,0,0,1);
	float frac = 1;
	vec3 P = rayOrigin.xyz;
	vec3 V = rayDirection.xyz;
	int depth = 0;
	while(depth < 6){
		int triangle = -1;
		vec3 tuv;
		vec4 fracColor;
		if(ray_trace_loop(P, V, MAX_T, rootSceneNode, tuv, triangle)){
			P = P + tuv.x * V;
			vec3 N;
			vec4 fragCol;
			Material material;
			getHitPayload(triangle, tuv, fragCol, N, material);
			fracColor = shadeFragment(P, V, fragCol, N, material);
			if(depth == 0)
				t = tuv.x;
			if(depth>=MAX_DEPTH) 
				fracColor[3] = 1; // cancel recursion by setting alpha to one
			depth++;
			// compute transparent component
			float alpha = fracColor[3];
			color += frac*alpha*fracColor;
			frac *= (1-alpha);
			if(alpha >= 0.99f || frac < 0.05f) break;
		} else {
			// maybe environment map
			t = MAX_T;
			fracColor = vec4(3, 215, 252, 255) /255;
			color += frac*1*fracColor;
			break;
		}
	}
	return color;
}

void main() {
	debugColor = vec4(0,0,0,0);
	// from https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-generating-camera-rays/generating-camera-rays
	vec3 rayOrigin, rayDirection;
	generatePixelRay(rayOrigin, rayDirection);

	int triangle_index = -1;

	float t_hit;
	outColor = rayTrace(rayOrigin, rayDirection, t_hit);
	if(displayTriangles != 0) {
		debugColor = vec4(0,0,0,1);
	}
	if(displayIntersectionT != 0) {
		debugColor = vec4(hsv2rgb(vec3(min(t_hit/50,0.66f),1,1)),1);
	} 
	if (displayTraversalDepth != 0){
		debugColor = vec4(hsv2rgb(vec3(0.66f - min(maxDepth * 1.f /colorSensitivity,0.66f),1,1)),1);
	}
	if (displayTraversalCount != 0){
		debugColor = vec4(hsv2rgb(vec3(0.66f - min(numTraversals * 1.f/colorSensitivity,0.66f),1,1)),1);
	}

	if(displayLights != 0) {
		int closestLight = -1;
		float light_t = t_hit;
		for(int i = 0;i<numLights;i++){
			Light light = lights[i];
			if((light.type & LIGHT_TYPE_POINT) != 0){
				float t;
				if(vertexIntersect(rayOrigin, rayDirection, light.position, t)){
					if(t<light_t){
						light_t = t;
						closestLight = i;
					}
				}
			}
		}
		if(closestLight>=0){
			Light light = lights[closestLight];
			if((light.type & LIGHT_ON) != 0){
				debugColor = vec4(1,1,1,1);
			} else {
				debugColor = vec4(0,0,0,1);
			}
		} else {
			debugColor = outColor;
		}
	}
	if(displayQueryCount != 0 && queryCount > 1){
		debugColor = vec4(hsv2rgb(vec3(0.66f - min(queryCount * 1.f /colorSensitivity,0.66f),1,1)),1);
	}

	if(debug != 0 && debugColor[3] == 1) outColor = debugColor;
}