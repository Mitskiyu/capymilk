typedef struct {
    f32 x, y, z;
} vec3_t;

typedef struct {
    f32 v[9]; // row-major
} mat3_t;

internal vec3_t mat3_mul_vec3(mat3_t m, vec3_t v);
