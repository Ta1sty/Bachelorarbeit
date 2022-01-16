#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#define RAY_TRACE
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
	uint recordQueryTrace; // if set to one records the querys
	uint pixelX;			// for the pixel with this X
	uint pixelY;			// and this Y coordinate
	uint traceMax;			// max amount (buffer size)
};

#ifdef RAY_TRACE

struct QueryTrace{
	vec3 start;
	float pad1;
	vec3 end;
	uint nodeNumber;
	uint isValid; // valid if 1, otherwise invalid
	uint tlasNumber; // the nodeNumber for this
	uint triangleIntersections;
	uint instanceIntersections;
};

layout(binding = 10, set = 3) uniform accelerationStructureEXT[] tlas;
layout(binding = 11, set = 3) buffer QueryTraceBuffer{ QueryTrace[] queryTraces; };
#endif

layout(location = 0) in vec3 fragColor;
vec4 debugColor; // if last is 0 it was never set, in that case use outColor
bool debugSetEnabled = true;
layout(location = 0) out vec4 outColor;

//https://gist.github.com/983/e170a24ae8eba2cd174f
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void SetDebugHsv(uint option, float number, float range, bool clampValue){
	if(option == 0) return;
	if(!debugSetEnabled) return;
	float x = number / range;
	if(clampValue)
		x = min(x,0.66);
	debugColor = vec4(hsv2rgb(vec3(x,1,1)),1);
}

void SetDebugCol(uint option, vec4 color){
	if(option == 0) return;
	if(!debugSetEnabled) return;
	debugColor = color;
}

void DebugOffIfSet(){
	if(debugColor[3] == 1)
		debugSetEnabled = false;
}


bool intersectAABB(vec3 rayOrigin, vec3 rayDir, SceneNode node, out float tNear, out float tFar) {
	vec3 boxMin = node.AABB_min.xyz;
	vec3 boxMax = node.AABB_max.xyz;
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    tNear = max(max(t1.x, t1.y), t1.z);
    tFar = min(min(t2.x, t2.y), t2.z);
	if(tNear>tFar)
		return false;
	return true;
}
float aabbT = 500;
void debugAABB(vec3 rayOrigin, vec3 rayDir, SceneNode node) {
	vec3 boxMin = node.AABB_min.xyz;
	vec3 boxMax = node.AABB_max.xyz;
	float tNear, tFar;
	if(!intersectAABB(rayOrigin, rayDir, node, tNear, tFar)) return;
	if(displayAABBs != 0){
		vec3 extent = boxMax - boxMin;
		vec3 P1 = rayOrigin + tNear * rayDir;
		vec3 P2 = rayOrigin + tFar * rayDir;

		float t[2] = {tNear, tFar};

		for(int i = 0;i<2;i++){
			if(t[i] < 0.001|| t[i] > aabbT) continue;
			vec3 P = rayOrigin + t[i] * rayDir;
			
			vec3 diffMax = (boxMax - P)/extent; // normalisieren

			int count = 0;
			if(diffMax.x < 0.01 || diffMax.x > 0.99)
				count++;
			if(diffMax.y < 0.01 || diffMax.y > 0.99)
				count++;
			if(diffMax.z < 0.01 || diffMax.z > 0.99)
				count++;
			if(count >= 2) {
				SetDebugHsv(displayAABBs, node.tlasNumber, colorSensitivity, true);
				aabbT = t[i];
			}
		}
	}
};

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

bool recordTrace = false;
uint nextRecord = 0;
void startTraceRecord(){
	ivec2 xy = ivec2(gl_FragCoord.xy);
	if(recordQueryTrace != 0 && xy.x == pixelX && xy.y == pixelY){
		recordTrace = true;
		nextRecord = 0;
	}
}
void recordQuery(uint nodeNumber, uint tlasNumber, vec3 start, vec3 end, uint triangleIntersections, uint instanceIntersections){ // records the 
	if(!recordTrace || nextRecord >= traceMax - 2) return;
	QueryTrace add;
	add.start = start;
	add.end = end;
	add.tlasNumber = tlasNumber;
	add.nodeNumber = nodeNumber;
	add.isValid = 1;
	add.triangleIntersections = triangleIntersections;
	add.instanceIntersections = instanceIntersections;
	queryTraces[nextRecord] = add;
	// add

	nextRecord++;
}
void checkQueryTrace(vec3 origin, vec3 direction){
	if(debug == 0 || displayQueryTrace == 0) return;
	for(int i = 0;i<traceMax;i++){
		QueryTrace trace = queryTraces[i];
		if(trace.isValid != 1) break; // is valid?
		vec3 E1 = normalize(trace.end - trace.start);
		vec3 E2 = normalize(direction);
		vec3 N = cross(E2, E1);
		float d = dot(N, trace.start - origin);
		if(abs(d)>0.005f) continue; // intersection is not near enough
		mat3 M = mat3(E1, -E2, N);
		mat3 MInv = inverse(M);
		vec3 ts = MInv * (origin-trace.start);
		float maxT = length(trace.end - trace.start);
		if(ts[0]<0 || ts[1]<0 || ts[0] > maxT) continue; // outide of bounds
		vec3 P = trace.start + ts[0] * E1;
		float dst = length(P-origin);
		if(dst < 0.005 || dst > 50) continue; // to close to the display or to far away
		SetDebugHsv(displayQueryTrace, trace.tlasNumber, colorSensitivity, false);
		// debugColor = vec4(hsv2rgb(vec3(0,1,1)),1);
	}
}
void endRecord(){
	if(!recordTrace) return;
	// adds an invalid record at the end of the list
	QueryTrace end;
	end.isValid = 0;
	queryTraces[nextRecord] = end;
}

void getHitPayload(int triangle, vec3 tuv, out vec4 color, out vec3 N, out Material material){
	Vertex v0 = vertices[indices[triangle * 3]];
	Vertex v1 = vertices[indices[triangle * 3 + 1]];
	Vertex v2 = vertices[indices[triangle * 3 + 2]];

	float w = 1 - tuv.y - tuv.z;
	float u = tuv.y;
	float v = tuv.z;
	

	// compute interpolated Normal and Tex - important this is in object space -> need to transform either 
	N = w * v0.normal + v * v1.normal + u * v2.normal;
	N = normalize(N);
	vec2 tex = w * v0.tex_coord + v * v1.tex_coord + u * v2.tex_coord;

	// get material and texture properties, if there are not set use default values
	if(v0.material_index < 0 || renderTextures == 0){
		material.k_a = 0.15f;
		material.k_d = 0.5f;
		material.k_s = 0.35f;
		material.texture_index = -1;
		if(renderTextures == 0)
			color = vec4(0.8,0.8,0.8,1);
		else
			color = vec4(0.2,0.4,0.8,1);
	} else {
		material = materials[v0.material_index];
		if(material.texture_index < 0) {
			color = vec4(0.2,0.4,0.8,1);
		}
		else {
			color = texture(sampler2D(textures[material.texture_index], samp), tex);
		}
	}

	// debug
	if(debug == 0 || debugColor[3] != 0) return;

	SetDebugCol(displayUV, vec4(u,v,0,1));
	SetDebugCol(displayTex, vec4(tex.x,tex.y,0,1));
	SetDebugHsv(displayTriangleIdx, triangle, colorSensitivity, false);
	if(v0.material_index >= 0)
		SetDebugHsv(displayMaterialIdx, v0.material_index, colorSensitivity, true);
	else 
		SetDebugCol(displayMaterialIdx, vec4(0.2,0.4,0.8,1));

	if(material.texture_index >= 0)
		SetDebugHsv(displayTextureIdx, material.texture_index, colorSensitivity, true);
	else 
		SetDebugCol(displayTextureIdx, vec4(0.2,0.4,0.8,1));
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
void instanceHit(rayQueryEXT ray_query, TraversalPayload load, SceneNode node, out float tNear, out float tFar) {
	float t = rayQueryGetIntersectionTEXT(ray_query, false);
	//if(t>best_t) return; // we already found a triangle outside this aabb before this was even hit we dont need to add this instance
	// this can never happen since best_t is our t_max for this query
	uint iIdx = rayQueryGetIntersectionInstanceIdEXT(ray_query, false);
	uint cIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, false);
	uint pIdx = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, false);
	vec3 origin = rayQueryGetIntersectionObjectRayOriginEXT(ray_query, false);
	vec3 direction = rayQueryGetIntersectionObjectRayDirectionEXT(ray_query, false);
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



	// here the shader adds the next payloads
	TraversalPayload nextLoad;
	nextLoad.transformed_origin = (next.world_to_object * vec4(origin,1)).xyz;
	nextLoad.transformed_direction = (next.world_to_object * vec4(direction,0)).xyz;
	nextLoad.node_idx = next.Index;

	intersectAABB(origin, direction, next, tNear, tFar);

	nextLoad.t = tNear;
	traversalBuffer[stackSize] = nextLoad;
	stackSize++;

	if(displayAABBs != 0)
		debugAABB(origin, direction, next);			

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


int traversalDepth = 0;
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
	start.t = 0;

	if(displayAABBs != 0)
		debugAABB(rayOrigin, rayDirection, nodes[root]);

	traversalBuffer[0] = start;

	triangle_index = -1;
	stackSize = 1;

	uint triangleTLAS = -1;
	while(stackSize>0){
		TraversalPayload load = traversalBuffer[stackSize-1];
		stackSize--; // remove last element
		if(load.t > best_t) continue;// there was already a closer hit, we can skip this one

		int start = stackSize;
		SceneNode node = nodes[load.node_idx]; // retrieve scene Node
		traversalDepth = max(node.level, traversalDepth); // not 100% correct but it gives a vague idea
		uint tlasNumber = node.tlasNumber;
        vec3 query_origin = load.transformed_origin;
        vec3 query_direction = load.transformed_direction;

		if(debug != 0 && displayAABBs != 0){
			for(int i = 0;i<node.numOdd;i++){
				SceneNode directChild = nodes[node.childrenIndex + i];
				debugAABB(load.transformed_origin, load.transformed_direction, directChild);
			}
		}

		rayQueryEXT ray_query;
		rayQueryInitializeEXT(ray_query, tlas[tlasNumber], 0, 0xFF, 
			query_origin, min_t, 
			query_direction, best_t);
		queryCount++;
		numTraversals++;
		uint triangleIntersections = 0;
		uint instanceIntersections = 0;

		float instanceTMin = 100000;
		float instanceTMax = 0;

		while(rayQueryProceedEXT(ray_query)){
			uint type = rayQueryGetIntersectionTypeEXT(ray_query, false);
			switch(type){
				case gl_RayQueryCandidateIntersectionTriangleEXT:
					// might want to check for opaque
					triangleHit(ray_query, node);
					triangleIntersections++;
					break;
				case gl_RayQueryCandidateIntersectionAABBEXT:
					// we do not want to generate intersections, since AABB hit does not guarantee a hit in the traversal
					// instead call instanceShader and add the new parameters to the traversalList
					// we need to pay attention! the query goes from smallest to largest t value and adds these to the list
					// for continuing traversal it is therfore beneficial to resume with the closest node, which was the one
					// that replaces the node that executed the query, however we still want to do a DFS. Solution for this is:
					// We reverse the list. Then the closest hit is at the end of the stack, and all added levels are still 
					// right of this node
					float tNear, tFar;
					instanceHit(ray_query, load, node, tNear, tFar);
					instanceIntersections++;
					instanceTMin = min(instanceTMin, tNear); 
					instanceTMax = max(instanceTMax, tFar); 

					break;
				default: break;
			}
		}
		int end = stackSize;
		int added = end - start;
		for(int i = 0;i<added/2;i++){
			TraversalPayload tmp1 = traversalBuffer[start+i];
			TraversalPayload tmp2 = traversalBuffer[end-1-i];
			traversalBuffer[start+i] = tmp2;
			traversalBuffer[end-1-i] = tmp1;
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
			//recordQuery(node.Index, rayOrigin+load.t * rayDirection, rayOrigin + best_t * rayDirection, triangleIntersections, instanceIntersections);
		}

		recordQuery(node.Index, node.tlasNumber, rayOrigin, rayOrigin + best_t * rayDirection, triangleIntersections, instanceIntersections);
	}
	SetDebugHsv(displayTLASNumber, triangleTLAS, colorSensitivity, true);
	if(triangle_index >= 0) {
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
			// for showing triangles we use anyHit and early out
			if(debug != 0 && displayTriangles != 0){
				vec2 uv = tuv_next.yz;
				if(uv.x+uv.y>0.99f || uv.x < 0.01f || uv.y < 0.01f) {
					return true;
				}
			} else {
				if(tuv_next.x < tuv.x) { // compute closest hit
					vec3 N;
					vec4 color;
					Material material;
					getHitPayload(i, tuv_next, color, N, material);
					if(color[3] == 0) // test that the hit is opque, else skip it
						continue;
					triangle_index = i;
					tuv = tuv_next;
				}
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
		float specular = 0;
		float diffuse = material.k_d;
		vec3 R = normalize(reflect(V, N));
		if(renderShadows != 0){
			if((light.type & LIGHT_TYPE_POINT) != 0){ // point light, check if it is visible and then compute KS via L vector
				vec3 LN = normalize(L);
				if(ray_trace_loop(P, LN, l_dst,rootSceneNode, tuvShadow, index)){ // is light source visible? shoot ray to lPos
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
		}

		// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff

		float d = l_dst - light.radius;
		if(renderDiffuse == 0) diffuse = 0;
		if(renderSpecular == 0) specular = 0;
		if(d<0)
			sum+=vec3(1);
		else {
			float l_mult = light.quadratic.x + light.quadratic.y / d + light.quadratic.z / (d*d);
			sum += (specular+diffuse) * l_mult * light.intensity * color.xyz;
		}
	}
	if(renderAmbient == 0) material.k_a = 0;
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
const float MAX_T = 300;
vec4 rayTrace(vec3 rayOrigin, vec3 rayDirection, out float t) {
	vec4 color = vec4(0,0,0,1);
	float frac = 1;
	vec3 P = rayOrigin.xyz;
	vec3 V = rayDirection.xyz;
	int depth = 0;

	if(debug != 0 && displayTriangles != 0) {
		int triangle = -1;
		vec3 tuv;
		if(ray_trace_loop(rayOrigin, rayDirection, MAX_T, rootSceneNode, tuv, triangle))
			return vec4(1,1,1,1);
		else
			return vec4(0,0,0,1);
	}


	while(depth < 800){
		int triangle = -1;
		vec3 tuv;
		vec4 fracColor;
		if(ray_trace_loop(P, V, MAX_T, rootSceneNode, tuv, triangle)){
			P = P + tuv.x * V;
			vec3 N;
			vec4 fragCol;
			Material material;

			getHitPayload(triangle, tuv, fragCol, N, material);
			DebugOffIfSet();
			if(displayAABBs != 0)
				debugSetEnabled = false;

			Vertex v0 = vertices[indices[triangle * 3]];
			fracColor = shadeFragment(P, V, fragCol, N, material);
			if(depth == 0)
				t = tuv.x;
			depth++;
			if(depth>=rayMaxDepth) 
				fracColor[3] = 1; // cancel recursion by setting alpha to one
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
	startTraceRecord();
	outColor = rayTrace(rayOrigin, rayDirection, t_hit);
	endRecord();
	checkQueryTrace(rayOrigin, rayDirection);

	if(displayIntersectionT != 0) {
		debugColor = vec4(hsv2rgb(vec3(min(t_hit * 1.f/colorSensitivity,0.66f),1,1)),1);
	} 
	if (displayTraversalDepth != 0){
		debugColor = vec4(hsv2rgb(vec3(0.66f - min(traversalDepth * 1.f /colorSensitivity,0.66f),1,1)),1);
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

		}
	}
	if(displayQueryCount != 0 && queryCount > 1){
		debugColor = vec4(hsv2rgb(vec3(0.66f - min(queryCount * 1.f /colorSensitivity,0.66f),1,1)),1);
	}
	if(debug != 0 && debugColor[3] == 1) outColor = debugColor;
}