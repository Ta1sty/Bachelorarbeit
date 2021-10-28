#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_samplerless_texture_functions : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_nonuniform_qualifier : enable
struct ViewData
{
    vec3 origin;
    vec3 view_direction;
};
struct ScreenData
{
    int width;
    int height;
};
layout(set=0, binding = 0) uniform Data
{
    ViewData view_data;
    ScreenData screen_data;
} data; 

struct TraversalNode{
    int[6] next_indices; // LODs, points to the index of the next TLAS
    int numLOD;
};

layout(binding = 1) uniform accelerationStructureEXT[] tlas_arr; // [0] is top level AS
layout(binding = 2) readonly buffer TraversalNodeBuffer{ // how exaclty do uniform and readonly differ? 
	TraversalNode nodes[];
} traversal_node_buffer;

const int BUFFER_SIZE = 50;

int selectLOD(){
    return 0;
}

void get_ray_direction(ivec2 pixel, out vec3 dir) {
    // MAFS
}

void doShading(float bestT, int triangleIndex){

}

void main() {
	ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec3 dir = vec3(0);
    get_ray_direction(pixel, dir);

    // run rayquery - move this whole construct to its own method->so can use it for shading and stuff
    int queueStart; // points to next element to be removed
    int queueEnd; // points to next free spot
    int queueElements; // currentNumber of elements in the queue, need this to detect buffer overflow and/or loop termination
    int tlas_index_arr[BUFFER_SIZE];
    vec3 transformed_ray_origin[BUFFER_SIZE];
    vec3 transformed_ray_direction[BUFFER_SIZE];

    queueStart = 0;
    queueEnd = 1;
    queueElements = 1;
    tlas_index_arr[0] = 0; // we define the first index in the tlas array to be the top_level_tlas
    transformed_ray_origin[0] = vec3(data.view_data.origin); // start with view pos in world space
    transformed_ray_direction[0] = vec3(dir.xyz); // and the pixel ray from world space

    int tlas_index;
    vec3 ray_orign;
    vec3 ray_direction;

    float bestT = 10000;
    int triangleIndex = -1;

    while(queueElements>0)
    {
        // remove an element
        tlas_index = tlas_index_arr[queueStart];
        ray_orign = transformed_ray_origin[queueStart];
        ray_direction = transformed_ray_direction[queueStart];
        queueStart = (queueStart + 1) % BUFFER_SIZE; // advance positon by one
        queueElements--; // decrement number of elements

        float min_t = 1.0e-3f;
        float max_t = 0x7F7FFFFF; // 2139095039, does max_t work with infinity? read set tmax only to 10000, uintBitsToFloat()
        rayQueryEXT ray_query;
		rayQueryInitializeEXT(ray_query, tlas_arr[tlas_index], 0, 0xFF, ray_orign, min_t, ray_direction, max_t);
        while(rayQueryProceedEXT(ray_query)){
            if(rayQueryGetIntersectionTypeEXT(ray_query, false) == gl_RayQueryCandidateIntersectionAABBEXT ){ // instance intersection
                int index = rayQueryGetIntersectionInstanceIdEXT(ray_query, false);
                // KP wie ich an die aabb instance komme, ich nehme einfach mal an das ich mir die als uniform buffer mitgeben soll oder so
                TraversalNode node = traversal_node_buffer.nodes[index]; // There is literally no way that this will work like this. ASK
                int LOD = selectLOD();
                tlas_index_arr[queueEnd] = node.next_indices[LOD];
                
                // Make sure intersectionObject... are really object not collision coordinates
                transformed_ray_origin[queueEnd] = rayQueryGetIntersectionObjectRayOriginEXT(ray_query, false);
                transformed_ray_direction[queueEnd] = rayQueryGetIntersectionObjectRayDirectionEXT(ray_query, false);
                queueEnd++;
                queueElements++;
                if(queueElements>BUFFER_SIZE) return; // handle this
            } else if(rayQueryGetIntersectionTypeEXT(ray_query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) { // triangle intersection
                rayQueryConfirmIntersectionEXT(ray_query); // might want to check for opaque
            }
        }

        // we only commit triangle intersections(for now, and later instances but never traversalNode intersections)
        if(rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT ) {
            float t = rayQueryGetIntersectionTEXT(ray_query, true);
            if(t<bestT){
                bestT = t;
                triangleIndex = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, true);
            }
        }
    }

    doShading(bestT, triangleIndex);
}
