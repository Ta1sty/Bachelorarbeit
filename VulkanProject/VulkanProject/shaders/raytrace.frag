
#ifndef STRUCTS
#include "structs.frag"
#endif

#ifndef MATH
#include "math.frag"
#endif

#ifdef RAY_QUERIES
#include "QueryTraceRecord.frag"
layout(binding = TLAS_BINDING, set = 3) uniform accelerationStructureEXT[] tlas;
#endif


void getHitPayload(int triangle, vec3 tuv, out vec4 color, out vec3 N, out Material material) {
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
	if (v0.material_index < 0 || !renderTextures) {
		material.k_a = 0.15f;
		material.k_d = 0.5f;
		material.k_s = 0.35f;
		material.texture_index = -1;
		if (!renderTextures)
			color = vec4(0.8, 0.8, 0.8, 1);
		else
			color = vec4(0.2, 0.4, 0.8, 1);
	}
	else {
		material = materials[v0.material_index];
		if (material.texture_index < 0) {
			color = vec4(0.2, 0.4, 0.8, 1);
		}
		else {
			color = texture(sampler2D(textures[material.texture_index], samp), tex);
		}
	}

	// debug
	if (!debug || debugColor[3] != 0) return;

	SetDebugCol(displayUV, vec4(u, v, 0, 1));
	SetDebugCol(displayTex, vec4(tex.x, tex.y, 0, 1));
	SetDebugHsv(displayTriangleIdx, triangle, colorSensitivity, false);
	if (v0.material_index >= 0)
		SetDebugHsv(displayMaterialIdx, v0.material_index, colorSensitivity, true);
	else
		SetDebugCol(displayMaterialIdx, vec4(0.2, 0.4, 0.8, 1));

	if (material.texture_index >= 0)
		SetDebugHsv(displayTextureIdx, material.texture_index, colorSensitivity, true);
	else
		SetDebugCol(displayTextureIdx, vec4(0.2, 0.4, 0.8, 1));
}


// ONLY modify these when in the instanceShader or the rayTrace Loop
struct TraversalPayload {
	mat4x3 world_to_object;
	float t; // the t for which this aabb was intersected
	uint nIdx; // node index, or custom index(aka, direct child) before compute
	uint pIdx;
	uint sIdx; // shader offset (dont use it), we use it instead to get the grandchild when using instance Lists
};

#ifdef RAY_QUERIES
// use a stack because:
// keep the list as short as possible, I.E. use a depth first search
const int BUFFER_SIZE = 30;
int stackSize = 0;
TraversalPayload traversalBuffer[BUFFER_SIZE];

int traversalDepth = 0;
uint numTraversals = 0;
uint queryCount = 0;

void instanceHitCompute(int index, vec3 rayOrigin, vec3 rayDirection, bool IsInstanceList){	
	TraversalPayload nextLoad = traversalBuffer[index];
	SceneNode directChild = nodes[nextLoad.nIdx]; // use the custom index to take a shortcut
	mat4 world_to_object;

	// we can now do LOD or whatever we feel like doing - currently InstanceList Implementation
	SceneNode next;
	if(IsInstanceList) {
		SceneNode instancedChild = nodes[nextLoad.sIdx];
		next = nodes[childIndices[instancedChild.childrenIndex + nextLoad.pIdx]]; // this is the instance 
																			  // - odd (absorbed as AABB in directChild)
		world_to_object = instancedChild.world_to_object * directChild.world_to_object;
	} else {
		next = nodes[childIndices[directChild.childrenIndex + nextLoad.pIdx]];
		world_to_object = directChild.world_to_object;
	}


	// here the shader adds the next payloads

	vec3 origin = (world_to_object * vec4(nextLoad.world_to_object * vec4(rayOrigin,1),1)).xyz;
	vec3 direction = (world_to_object * vec4(nextLoad.world_to_object * vec4(rayDirection,0),0)).xyz;

	float tNear, tFar;
	intersectAABB(origin, direction, next.AABB_min.xyz, next.AABB_max.xyz, tNear, tFar);

	world_to_object = next.world_to_object * world_to_object;

	nextLoad.nIdx = next.Index;
	nextLoad.world_to_object = mat4x3(world_to_object * mat4(nextLoad.world_to_object));
	nextLoad.t = tNear;
	traversalBuffer[index] = nextLoad;
	
	if (debug && displayAABBs) {
		debugAABB(origin, direction, next);
	}
}


void triangleHit(rayQueryEXT ray_query, SceneNode node) {
	float t = rayQueryGetIntersectionTEXT(ray_query, false);
	vec2 uv = rayQueryGetIntersectionBarycentricsEXT(ray_query, false);
	uint cIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, false);
	SceneNode blasChild = nodes[cIdx];
	int triangle = blasChild.IndexBuferIndex / 3 + rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, false);
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
	if (color[3] > 0)
		rayQueryConfirmIntersectionEXT(ray_query);
}

bool ray_trace_loop(vec3 rayOrigin, vec3 rayDirection, float t_max, uint root, out vec3 tuv, out int triangle_index, out TraversalPayload resultPayload) {
	numTraversals = 0;

	tuv = vec3(0);

	float min_t = 1.0e-3f;
	float best_t = t_max;

	TraversalPayload start; // start at root node
	start.world_to_object = mat4x3(1);
	start.nIdx = root;
	start.t = 0;

	if (debug && displayAABBs) {
		SceneNode root = nodes[root];
		debugAABB(rayOrigin, rayDirection, root);
	}

	traversalBuffer[0] = start;

	triangle_index = -1;
	stackSize = 1;

	uint triangleTLAS = -1;
	while (stackSize > 0) {
		stackSize--; // remove last element
		TraversalPayload load = traversalBuffer[stackSize];
		if (load.t > best_t) continue;// there was already a closer hit, we can skip this one

		int start = stackSize;
		SceneNode node = nodes[load.nIdx]; // retrieve scene Node
		traversalDepth = max(node.level, traversalDepth); // not 100% correct but it gives a vague idea

		uint tlasNumber = node.tlasNumber;
		vec3 query_origin = (load.world_to_object * vec4(rayOrigin,1)).xyz;
		vec3 query_direction = (load.world_to_object * vec4(rayDirection,0)).xyz;

		if (debug && displayAABBs) {
			for (int i = 0; i < node.NumChildren; i++) {
				SceneNode directChild = nodes[childIndices[node.childrenIndex + i]];
				debugAABB(query_origin, query_direction, directChild);
			}
			if(node.IsInstanceList){
				if(displayListAABBs){
					SceneNode list = nodes[childIndices[node.childrenIndex]];
					for (int i = 0; i < min(50,list.NumChildren); i++) {
						SceneNode child = nodes[childIndices[list.childrenIndex + i]];
						debugAABB(query_origin, query_direction, child);
					}
				}
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

		while (rayQueryProceedEXT(ray_query)) {
			uint type = rayQueryGetIntersectionTypeEXT(ray_query, false);
			switch (type) {
			case gl_RayQueryCandidateIntersectionTriangleEXT:
				// might want to check for opaque
				// triangleHit(ray_query, node);
				rayQueryConfirmIntersectionEXT(ray_query);
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
				if(stackSize>=BUFFER_SIZE)
					break;

				traversalBuffer[stackSize] = load;
				traversalBuffer[stackSize].nIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, false);
				traversalBuffer[stackSize].pIdx = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, false);
				traversalBuffer[stackSize].sIdx = rayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetEXT(ray_query, false);
				stackSize++;
				instanceIntersections++;
				break;
			default: break;
			}
		}
		int end = stackSize;
		int added = end - start;
		for (int i = 0; i < added / 2; i++) {
			int i1 = start + i;
			int i2 = end - 1 - i;
			TraversalPayload tmp1 = traversalBuffer[i1];
			TraversalPayload tmp2 = traversalBuffer[i2];
			traversalBuffer[i1] = tmp2;
			traversalBuffer[i2] = tmp1;
		}

		for(int i = start;i < end;i++) {
			instanceHitCompute(i, rayOrigin, rayDirection, node.IsInstanceList);
		}

		uint commitedType = rayQueryGetIntersectionTypeEXT(ray_query, true);
		if (commitedType == gl_RayQueryCommittedIntersectionTriangleEXT) {
			if (debug && displayTriangles) {
				vec2 uv = rayQueryGetIntersectionBarycentricsEXT(ray_query, true);
				if (uv.x + uv.y > 0.99f || uv.x < 0.01f || uv.y < 0.01f) {
					triangle_index = 0;
					return true;
				}
				else {
					continue;
				}
			}
			float t = rayQueryGetIntersectionTEXT(ray_query, true);
			if (t < best_t) {
				best_t = t;

				uint cIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, true);
				SceneNode blasChild = nodes[cIdx];
				triangle_index = blasChild.IndexBuferIndex / 3 + rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, true);

				tuv.x = t;
				vec2 uv = rayQueryGetIntersectionBarycentricsEXT(ray_query, true);
				tuv.y = uv.y;
				tuv.z = uv.x;

				mat4 world_to_object = mat4(rayQueryGetIntersectionWorldToObjectEXT(ray_query, true));
				resultPayload.nIdx = blasChild.Index;
				resultPayload.world_to_object = mat4x3(world_to_object * mat4(load.world_to_object));
				resultPayload.t = best_t;

				triangleTLAS = tlasNumber;
			}
		}
		// recordQuery(node.Index, node.level, load.t, rayOrigin, rayOrigin + best_t * rayDirection, triangleIntersections ,instanceIntersections);
	}
	SetDebugHsv(displayTLASNumber, triangleTLAS, colorSensitivity, true);
	if (triangle_index >= 0) {
		return true;
	}
	return false;
}
#endif

#ifndef RAY_QUERIES
bool ray_trace_loop(vec3 rayOrigin, vec3 rayDirection, float t_max, uint root, out vec3 tuv, out int triangle_index, out TraversalPayload resultPayload) {
	vec3 tuv_next;
	tuv.x = t_max;
	triangle_index = -1;
	for (int i = 0; i < numTriangles; i++) {
		Vertex vert_v0 = vertices[indices[i * 3]];
		Vertex vert_v1 = vertices[indices[i * 3 + 1]];
		Vertex vert_v2 = vertices[indices[i * 3 + 2]];

		vec3 v0 = vert_v0.position;
		vec3 v2 = vert_v1.position;
		vec3 v1 = vert_v2.position;


		if (rayTriangleIntersect(rayOrigin, rayDirection, tuv_next, v0, v1, v2)) {
			// for showing triangles we use anyHit and early out
			if (debug != 0 && displayTriangles != 0) {
				vec2 uv = tuv_next.yz;
				if (uv.x + uv.y > 0.99f || uv.x < 0.01f || uv.y < 0.01f) {
					return true;
				}
			}
			else {
				if (tuv_next.x < tuv.x) { // compute closest hit
					vec3 N;
					vec4 color;
					Material material;
					getHitPayload(i, tuv_next, color, N, material);
					if (color[3] == 0) // test that the hit is opque, else skip it
						continue;
					triangle_index = i;
					tuv = tuv_next;
				}
			}
		}
	}

	if (triangle_index == -1) {
		return false;
	}
	return true;
}
#endif


vec4 shadeFragment(vec3 P, vec3 V, vec3 N, vec4 color, Material material) {
	float n = 1; // todo phong exponent
	// calculate lighting for each light source
	vec3 sum = vec3(0);
	for (int i = 0; i < numLights; i++) {
		Light light = lights[i];

		if ((light.type & LIGHT_ON) == 0) { // is the light on?
			continue;
		}
		vec3 tuvShadow;
		int index;

		vec3 L = light.position - P;
		float l_dst = length(L);
		if (l_dst > light.maxDst && (light.type & LIGHT_IGNORE_MAX_DISTANCE) == 0) { // is the light near enough to even matter
			continue;
		}
		float specular = 0;
		float diffuse = material.k_d;
		vec3 R = normalize(reflect(V, N));
		if (renderShadows) {
			TraversalPayload load;
			if ((light.type & LIGHT_TYPE_POINT) != 0) { // point light, check if it is visible and then compute KS via L vector
				vec3 LN = normalize(L);
				if (ray_trace_loop(P, LN, l_dst, rootSceneNode, tuvShadow, index, load)) { // is light source visible? shoot ray to lPos
					continue;
				}
				specular = material.k_s * pow(max(0, dot(R, LN)), n);
			}
			else if ((light.type & LIGHT_TYPE_DIRECTIONAL) != 0) { // directional light, check if it is visible 
				if ((light.type & LIGHT_USE_MIN_DST) != 0) { // shoot ray over minDst, if nothing is hit lighting is enabled
					if (ray_trace_loop(P, normalize(-light.direction), light.minDst, rootSceneNode, tuvShadow, index, load)) { // shoot shadow ray into the light source
						continue;
					}
					specular = material.k_s * pow(max(0, dot(R, -light.direction)), n);
				}
				else { // directional light with fixed position, this is probably not correct
					if (ray_trace_loop(P, normalize(-light.direction), l_dst, rootSceneNode, tuvShadow, index, load)) {
						continue;
					}
					specular = material.k_s * pow(max(0, dot(R, -light.direction)), n);
				}
			}
		}

		// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff

		float d = l_dst - light.radius;
		if (!renderDiffuse) diffuse = 0;
		if (!renderSpecular) specular = 0;
		if (d < 0)
			sum += vec3(1);
		else {
			float l_mult = light.quadratic.x + light.quadratic.y / d + light.quadratic.z / (d * d);
			sum += (specular + diffuse) * l_mult * light.intensity * color.xyz;
		}
	}
	if (!renderAmbient) material.k_a = 0;
	sum += material.k_a * color.xyz;
	return vec4(sum.xyz, color[3]);
}

const float MAX_T = 100000;
vec4 rayTrace(vec3 rayOrigin, vec3 rayDirection, out float t) {
	vec4 color = vec4(0, 0, 0, 1);
	float frac = 1;
	vec3 P = rayOrigin.xyz;
	vec3 V = rayDirection.xyz;
	int depth = 0;

	if (debug && displayTriangles) {
		int triangle = -1;
		TraversalPayload load;
		vec3 tuv;
		if (ray_trace_loop(rayOrigin, rayDirection, MAX_T, rootSceneNode, tuv, triangle, load))
			return vec4(1, 1, 1, 1);
		else
			return vec4(0, 0, 0, 1);
	}


	while (depth < 800) {
		int triangle = -1;
		TraversalPayload load;
		vec3 tuv;
		vec4 fracColor;
		if (ray_trace_loop(P, V, MAX_T, rootSceneNode, tuv, triangle, load)) {
			P = P + tuv.x * V;
			vec3 N_obj;
			vec4 fragCol;
			Material material;

			getHitPayload(triangle, tuv, fragCol, N_obj, material);
			DebugOffIfSet();
			if (displayAABBs)
				debugSetEnabled = false;

			mat3 world_to_object = mat3(load.world_to_object);
			mat3 object_to_world = inverse(world_to_object);
			mat3 NormalTransform = transpose(inverse(object_to_world));
			vec3 N_world = NormalTransform * N_obj;

			fracColor = shadeFragment(P, V, N_world, fragCol, material);
			if (depth == 0)
				t = tuv.x;
			depth++;
			if (depth >= rayMaxDepth)
				fracColor[3] = 1; // cancel recursion by setting alpha to one
			// compute transparent component
			float alpha = fracColor[3];
			color += frac * alpha * fracColor;
			frac *= (1 - alpha);
			if (alpha >= 0.99f || frac < 0.05f) break;
		}
		else {
			// maybe environment map
			t = MAX_T;
			fracColor = vec4(3, 215, 252, 255) / 255;
			color += frac * 1 * fracColor;
			break;
		}
	}
	return color;
}


void generatePixelRay(out vec3 rayOrigin, out vec3 rayDirection) {
	ivec2 xy = ivec2(gl_FragCoord.xy);
	float x = xy.x;
	float y = xy.y;

	float flt_height = height;
	float flt_width = width;
	vec4 origin_view_space = vec4(0, 0, 0, 1);
	vec4 origin_world_space = view_to_world * origin_view_space;
	float z = flt_height / tan(PI / 180 * fov);
	vec4 direction_view_space = vec4(normalize(vec3((x - flt_width / 2.f), flt_height / 2.f - y, -z)), 0);
	vec4 direction_world_space = view_to_world * direction_view_space;

	rayOrigin = origin_world_space.xyz;
	rayDirection = direction_world_space.xyz;
}

