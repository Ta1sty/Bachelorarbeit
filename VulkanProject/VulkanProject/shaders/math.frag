

vec4 rotationFromQuaternion(vec3 u, vec3 v)
{
    vec3 w = cross(u, v);
    vec4 q = vec4(w.x, w.y, w.z, 1.f + dot(u, v));
    return normalize(q);
}

vec3 rotateByQuaternion(vec4 q, vec3 v) {
	return v + 2.0*cross(cross(v, q.xyz ) + q.w*v, q.xyz);
}


mat4 getTransform(vec4 q, vec3 t){
    mat4 mat;
     
    mat[0][0] = 2 * (q.w * q.w + q.x * q.x) - 1;
	mat[0][1] = 2 * (q.x * q.y + q.w * q.z);
	mat[0][2] = 2 * (q.x * q.z - q.w * q.y);

    mat[1][0] = 2 * (q.x * q.y - q.w * q.z);
	mat[1][1] = 2 * (q.w * q.w + q.y * q.y) - 1;
	mat[1][2] = 2 * (q.y * q.z + q.w * q.x);

    mat[2][0] = 2 * (q.x * q.z + q.w * q.y);
    mat[2][1] = 2 * (q.y * q.z - q.w * q.x);
    mat[2][2] = 2 * (q.w * q.w + q.z * q.z) - 1;

	mat[3][0] = t.x;
	mat[3][1] = t.y;
	mat[3][2] = t.z;

	mat[3][3] = 1;
	return mat;
}
mat4x3 inv(mat4x3 tr){
	mat3 rot = inverse(mat3(tr));
	return mat4x3(rot[0],rot[1],rot[2],-(rot*tr[3]));
}