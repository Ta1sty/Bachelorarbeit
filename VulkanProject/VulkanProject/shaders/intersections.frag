#define t_min 1.0e-4f
#define EPSILON 0.0000001
bool intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax, out float tNear, out float tFar) {
	vec3 tMin = (boxMin - rayOrigin) / rayDir;
	vec3 tMax = (boxMax - rayOrigin) / rayDir;
	vec3 t1 = min(tMin, tMax);
	vec3 t2 = max(tMin, tMax);
	tNear = max(max(t1.x, t1.y), t1.z);
	tFar = min(min(t2.x, t2.y), t2.z);
	if (tNear > tFar)
		return false;
	return true;
}

bool rayTriangleIntersect(vec3 rayOrigin, vec3 rayDirection, out vec3 tuv, vec3 v0, vec3 v1, vec3 v2) {
	vec3 edge1 = v1 - v0;
	vec3 edge2 = v2 - v0;
	vec3 h = cross(rayDirection, edge2);
	float a = dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;    // This ray is parallel to this triangle.
	float f = 1.0f / a;
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
		tuv = vec3(t, u, v);
		vec3 outIntersectionPoint = rayOrigin + rayDirection * t;
		return true;
	}
	else // This means that there is a line intersection but not a ray intersection.
		return false;
}
bool vertexIntersect(vec3 rayOrigin, vec3 rayDir, vec3 postion, out float t) {
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
	float t1 = (-b - disciminant) / (2 * a);
	float t2 = (-b + disciminant) / (2 * a);
	//this only works because a is strictly positive
	if (t2 < t_min) return false;
	if (t1 < t_min) {
		t = t2;
		return true;
	}
	t = t1;
	return true;
}