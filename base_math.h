typedef struct {
    f32 x, y, z;
} vec3;

typedef struct {
    f32 v[9]; // row-major
} mat3;

internal vec3 mat3_mul_vec3(mat3 m, vec3 v);
