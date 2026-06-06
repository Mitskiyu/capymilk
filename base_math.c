internal vec3_t mat3_mul_vec3(mat3_t m, vec3_t v) {
    vec3_t res;
    res.x = m.v[0]*v.x + m.v[1]*v.y + m.v[2]*v.z;
    res.y = m.v[3]*v.x + m.v[4]*v.y + m.v[5]*v.z;
    res.z = m.v[6]*v.x + m.v[7]*v.y + m.v[8]*v.z;

    return res;
}
