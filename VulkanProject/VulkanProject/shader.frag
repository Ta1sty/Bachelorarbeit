#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#define RAY_QUERIES
#ifdef RAY_QUERIES
#extension GL_EXT_ray_query : require
#endif		
#define PI 3.1415926538

#include "intersections.frag"

#define STRUCTS
#include "structs.frag"

#define DEBUG
#include "debug.frag"

#define RAYTRACE
#include "raytrace.frag"

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

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

	if(displayIntersectionT) {
		debugColor = vec4(hsv2rgb(vec3(min(t_hit * 1.f/colorSensitivity,0.66f),1,1)),1);
	} 
	if (displayTraversalDepth){
		debugColor = vec4(hsv2rgb(vec3(0.66f - min(traversalDepth * 1.f /colorSensitivity,0.66f),1,1)),1);
	}
	if (displayTraversalCount){
		debugColor = vec4(hsv2rgb(vec3(0.66f - min(numTraversals * 1.f/colorSensitivity,0.66f),1,1)),1);
	}

	if(displayLights) {
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
	if(displayQueryCount && queryCount > 1){
		debugColor = vec4(hsv2rgb(vec3(0.66f - min(queryCount * 1.f /colorSensitivity,0.66f),1,1)),1);
	}
	if(debug && debugColor[3] == 1) outColor = debugColor;
}