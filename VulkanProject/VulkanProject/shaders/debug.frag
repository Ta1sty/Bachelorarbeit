
#ifndef STRUCTS
#include "structs.frag"
#endif

vec4 debugColor; // if last is 0 it was never set, in that case use outColor
bool debugSetEnabled = true;

//https://gist.github.com/983/e170a24ae8eba2cd174f
vec3 hsv2rgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void SetDebugHsv(bool option, float number, float range, bool clampValue) {
	if (!option) return;
	if (!debugSetEnabled) return;
	float x = number / range;
	if (clampValue)
		x = min(x, 0.66);
	debugColor = vec4(hsv2rgb(vec3(x, 1, 1)), 1);
}

void SetDebugCol(bool option, vec4 color) {
	if (!option) return;
	if (!debugSetEnabled) return;
	debugColor = color;
}

void DebugOffIfSet() {
	if (debugColor[3] == 1)
		debugSetEnabled = false;
}


float aabbT = 500;
void debugAABB(vec3 rayOrigin, vec3 rayDir, SceneNode node) {
	vec3 boxMin = node.AABB_min;
	vec3 boxMax = node.AABB_max;
	float tNear, tFar;
	if (!intersectAABB(rayOrigin, rayDir, node.AABB_min, node.AABB_max, tNear, tFar)) return;
	if (displayAABBs) {
		vec3 extent = boxMax - boxMin;
		vec3 P1 = rayOrigin + tNear * rayDir;
		vec3 P2 = rayOrigin + tFar * rayDir;

		float t[2] = { tNear, tFar };

		for (int i = 0; i < 2; i++) {
			if (t[i] < 0.001 || t[i] > 100000) continue;
			vec3 P = rayOrigin + t[i] * rayDir;

			vec3 diffMax = (boxMax - P) / extent; // normalisieren

			int count = 0;
			if (diffMax.x < 0.01 || diffMax.x > 0.99)
				count++;
			if (diffMax.y < 0.01 || diffMax.y > 0.99)
				count++;
			if (diffMax.z < 0.01 || diffMax.z > 0.99)
				count++;
			if (count >= 2) {
				SetDebugHsv(displayAABBs, node.Index, colorSensitivity, false);
				aabbT = t[i];
			}
		}
	}
};
