
mat3 rotateAlign(vec3 v1, vec3 v2)
{
    vec3 axis = cross(v1, v2);

    const float cosA = dot(v1, v2);
    const float k = 1.0f / (1.0f + cosA);

    mat3 result = mat3((axis.x * axis.x * k) + cosA,
        (axis.y * axis.x * k) - axis.z,
        (axis.z * axis.x * k) + axis.y,
        (axis.x * axis.y * k) + axis.z,
        (axis.y * axis.y * k) + cosA,
        (axis.z * axis.y * k) - axis.x,
        (axis.x * axis.z * k) - axis.y,
        (axis.y * axis.z * k) + axis.x,
        (axis.z * axis.z * k) + cosA
    );

    return result;
}

vec4 rotationFromQuaternion(vec3 u, vec3 v)
{
    vec3 w = cross(u, v);
    vec4 q = vec4(w.x, w.y, w.z, 1.f + dot(u, v));
    return normalize(q);
}

vec3 rotateByQuaternion(vec3 v, vec4 q) {
    vec3 u = vec3(q.x, q.y, q.z);
    float s = q.w;
    return 2.0f * dot(u, v) * u
        + (s * s - dot(u, u)) * v
        + 2.0f * s * cross(u, v);
}
