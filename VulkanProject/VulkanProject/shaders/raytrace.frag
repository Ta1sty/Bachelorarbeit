
#ifndef STRUCTS
#include "structs.frag"
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
	mat4 world_to_object;
	vec3 transformed_origin; // ray origin in object space
	float t; // the t for which this aabb was intersected
	vec3 transformed_direction; // the ray origin in object space
	uint node_idx; // the SceneNode to which this traversalNode belongs
	vec3 transformed_x;
	float pad;
};

#ifdef RAY_QUERIES
// use a stack because:
// keep the list as short as possible, I.E. use a depth first search
const int BUFFER_SIZE = 50;
int stackSize = 0;
TraversalPayload traversalBuffer[BUFFER_SIZE];

int traversalDepth = 0;
uint numTraversals = 0;
uint queryCount = 0;

// Problem, want to reconstruct the tranform path for the Normal and potentially other applications
// Idea, have 2 arrays int[MAX_DEPTH] commited, candidate
// if triangle is accepted by traversal, copy contents of candidate to commited. This array then contains the levels used.
// in case there is a skip from exapmle level 3->6, we set 4,5 to -1 so we know to ignore them
// for traversal proceed as follows. once node is popped write node.index to candidate[node.level]
// if a path fails without any 

// for a given rayQuery this method returns a ray and a tlasNumber
void instanceHit(rayQueryEXT ray_query, TraversalPayload load, SceneNode node, out float tNear, out float tFar) {
	float t = rayQueryGetIntersectionTEXT(ray_query, false);
	// this can never happen since best_t is our t_max for this query
	uint iIdx = rayQueryGetIntersectionInstanceIdEXT(ray_query, false);
	uint cIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, false);
	uint pIdx = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, false);
	SceneNode directChild = nodes[cIdx]; // use the custom index to take a shortcut
	mat4 world_to_object;
	vec3 origin, direction, x;

	// we can now do LOD or whatever we feel like doing
	SceneNode next;
	if(directChild.IsInstanceList) {
		SceneNode dummy = nodes[childIndices[directChild.childrenIndex]]; // this is the inserted dummy - even
		SceneNode instance = nodes[childIndices[dummy.childrenIndex + pIdx]]; // this is the instance 
																			  // - odd (absorbed as AABB in directChild)
		next = nodes[childIndices[instance.childrenIndex]]; // instance is only allowed to reference one child, therefore this is the next
		world_to_object = instance.world_to_object * directChild.world_to_object;
	} else {
		next = nodes[childIndices[directChild.childrenIndex + pIdx]];
		world_to_object = directChild.world_to_object;
	}

	origin = (world_to_object * vec4(load.transformed_origin, 1)).xyz;
	direction = (world_to_object * vec4(load.transformed_direction, 0)).xyz;
	x = (world_to_object * vec4(load.transformed_x, 0)).xyz;

	// here the shader adds the next payloads
	TraversalPayload nextLoad;
	nextLoad.transformed_origin = (next.world_to_object * vec4(origin, 1)).xyz;
	nextLoad.transformed_direction = (next.world_to_object * vec4(direction, 0)).xyz;
	nextLoad.transformed_x = (next.world_to_object * vec4(x, 0)).xyz;
	nextLoad.node_idx = next.Index;
	nextLoad.world_to_object = next.world_to_object * world_to_object * load.world_to_object;

	intersectAABB(origin, direction, next.AABB_min.xyz, next.AABB_max.xyz, tNear, tFar);

	nextLoad.t = tNear;
	if(stackSize>=BUFFER_SIZE)
		return;
	traversalBuffer[stackSize] = nextLoad;
	stackSize++;

	if (displayAABBs)
		debugAABB(origin, direction, next);
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
	start.world_to_object = mat4(1);
	start.transformed_origin = rayOrigin;
	start.transformed_direction = rayDirection;
	start.transformed_x = vec3(1, 0, 0);
	start.node_idx = root;
	start.t = 0;

	if (displayAABBs)
		debugAABB(rayOrigin, rayDirection, nodes[root]);

	traversalBuffer[0] = start;

	triangle_index = -1;
	stackSize = 1;

	uint triangleTLAS = -1;
	while (stackSize > 0) {
		TraversalPayload load = traversalBuffer[stackSize - 1];
		stackSize--; // remove last element
		if (load.t > best_t) continue;// there was already a closer hit, we can skip this one


		int start = stackSize;
		SceneNode node = nodes[load.node_idx]; // retrieve scene Node
		traversalDepth = max(node.level, traversalDepth); // not 100% correct but it gives a vague idea

		// recordQuery(node.Index, node.level, load.t, rayOrigin, rayOrigin, 0, 0);

		uint tlasNumber = node.tlasNumber;
		vec3 query_origin = load.transformed_origin;
		vec3 query_direction = load.transformed_direction;

		if (debug && displayAABBs) {
			for (int i = 0; i < node.numOdd; i++) {
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

		while (rayQueryProceedEXT(ray_query)) {
			uint type = rayQueryGetIntersectionTypeEXT(ray_query, false);
			switch (type) {
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
		for (int i = 0; i < added / 2; i++) {
			TraversalPayload tmp1 = traversalBuffer[start + i];
			TraversalPayload tmp2 = traversalBuffer[end - 1 - i];
			traversalBuffer[start + i] = tmp2;
			traversalBuffer[end - 1 - i] = tmp1;
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
				resultPayload.transformed_origin = (world_to_object * vec4(load.transformed_origin, 1)).xyz;
				resultPayload.transformed_direction = (world_to_object * vec4(load.transformed_direction, 0)).xyz;
				resultPayload.transformed_x = (world_to_object * vec4(load.transformed_x, 0)).xyz;
				resultPayload.node_idx = blasChild.Index;
				resultPayload.world_to_object = world_to_object * load.world_to_object;
				resultPayload.t = best_t;

				triangleTLAS = tlasNumber;
			}
		}
	}
	SetDebugHsv(displayTLASNumber, triangleTLAS, colorSensitivity, true);
	if (triangle_index >= 0) {
		// recordQuery(999, 999, best_t, rayOrigin, rayOrigin + best_t * rayDirection, 999, 999);
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

