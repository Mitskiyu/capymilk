#include <stdlib.h>

global vec3 color_table[COLOR_TABLE_SIZE];

internal f32 glx_ex(const glx_params *params, f32 radius) {
    f32 rad_farfield = params->rad_galaxy * 2;

    if (radius < params->rad_core) {
        f32 t = radius / params->rad_core;
        return 1.0f + t * (params->ex_inner - 1.0f);
    } else if (radius < params->rad_galaxy) {
        f32 t = (radius - params->rad_core) / (params->rad_galaxy - params->rad_core);
        return params->ex_inner + t * (params->ex_outer - params->ex_inner);
    } else if (radius < rad_farfield) {
        f32 t = (radius - params->rad_galaxy) / (rad_farfield - params->rad_galaxy);
        return params->ex_outer + t * (1.0f - params->ex_outer);
    } else 
        return 1.0f;
}

internal f32 glx_brightness(f32 rad_core, f32 rad_galaxy, f32 radius) {
    f32 a = rad_galaxy / 3.0f;
    if (radius < rad_core) {
        return I0_BULGE * expf(-K * powf(radius, 0.25f));
    } else {
        f32 i0_disk = I0_BULGE * expf(-K * powf(rad_core, 0.25f));
        return i0_disk * expf(-(radius - rad_core) / a);
    }
}

internal void glx_cdf_build(f32 *radii, f32 *cumulative, f32 *inverse, f32 rad_core, f32 rad_galaxy) {
    f32 rad_farfield = rad_galaxy * 2;
    f32 step = rad_farfield / CDF_STEPS;
    f32 h = 1.0f / (CDF_POINTS - 1);

    f32 total = 0;
    for (i32 i = 0; i < CDF_POINTS; i++) {
        f32 l = (i*2) * step;
        f32 m = (i*2 + 1) * step;
        f32 r = (i*2 + 2) * step;
        total += step / 3 * (glx_brightness(rad_core, rad_galaxy, l) +
                             glx_brightness(rad_core, rad_galaxy, m) * 4 +
                             glx_brightness(rad_core, rad_galaxy, r));

        radii[i] = r;
        cumulative[i] = total;
    }

    // normalize
    f32 max = cumulative[CDF_POINTS - 1];
    for (i32 i = 0; i < CDF_POINTS; i++) {
        cumulative[i] /= max;
    }

    for (i32 i = 0; i < CDF_POINTS; i++) {
        f32 p = i * h;

        i32 lo = 0, hi = CDF_POINTS - 1;
        while (lo < hi) {
            i32 mid = lo + (hi - lo) / 2;
            if (p > cumulative[mid]) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        if (lo == 0) {
            inverse[i] = radii[0];
        }
        else {
            f32 t = (p - cumulative[lo-1]) / (cumulative[lo] - cumulative[lo-1]);
            inverse[i] = radii[lo-1] + t * (radii[lo] - radii[lo-1]);
        }
    }
}

internal f32 glx_cdf_sample(const f32 *inverse, f32 p) {
    f32 h = 1.0f / (CDF_POINTS - 1);
    i32 i = (i32)(p / h);
    if (i >= CDF_POINTS - 1)
        return inverse[CDF_POINTS-1];

    f32 t = (p - i * h) / h;
    return inverse[i] + t * (inverse[i+1] - inverse[i]);
}

internal f32 glx_bb_spectrum(i32 wavelength, f32 temp) {
    f32 wavelength_m = wavelength * 1e-9f;
    return PLANCK_C1 / powf(wavelength_m, 5) / (expf(PLANCK_C2 / (wavelength_m * temp)) - 1);
}

internal vec3 glx_spectrum_to_xyz(f32 temp) {
    local_persist const f32 cie_color_match[81][3] = {
        {0.0014f,0.0000f,0.0065f}, {0.0022f,0.0001f,0.0105f}, {0.0042f,0.0001f,0.0201f},
        {0.0076f,0.0002f,0.0362f}, {0.0143f,0.0004f,0.0679f}, {0.0232f,0.0006f,0.1102f},
        {0.0435f,0.0012f,0.2074f}, {0.0776f,0.0022f,0.3713f}, {0.1344f,0.0040f,0.6456f},
        {0.2148f,0.0073f,1.0391f}, {0.2839f,0.0116f,1.3856f}, {0.3285f,0.0168f,1.6230f},
        {0.3483f,0.0230f,1.7471f}, {0.3481f,0.0298f,1.7826f}, {0.3362f,0.0380f,1.7721f},
        {0.3187f,0.0480f,1.7441f}, {0.2908f,0.0600f,1.6692f}, {0.2511f,0.0739f,1.5281f},
        {0.1954f,0.0910f,1.2876f}, {0.1421f,0.1126f,1.0419f}, {0.0956f,0.1390f,0.8130f},
        {0.0580f,0.1693f,0.6162f}, {0.0320f,0.2080f,0.4652f}, {0.0147f,0.2586f,0.3533f},
        {0.0049f,0.3230f,0.2720f}, {0.0024f,0.4073f,0.2123f}, {0.0093f,0.5030f,0.1582f},
        {0.0291f,0.6082f,0.1117f}, {0.0633f,0.7100f,0.0782f}, {0.1096f,0.7932f,0.0573f},
        {0.1655f,0.8620f,0.0422f}, {0.2257f,0.9149f,0.0298f}, {0.2904f,0.9540f,0.0203f},
        {0.3597f,0.9803f,0.0134f}, {0.4334f,0.9950f,0.0087f}, {0.5121f,1.0000f,0.0057f},
        {0.5945f,0.9950f,0.0039f}, {0.6784f,0.9786f,0.0027f}, {0.7621f,0.9520f,0.0021f},
        {0.8425f,0.9154f,0.0018f}, {0.9163f,0.8700f,0.0017f}, {0.9786f,0.8163f,0.0014f},
        {1.0263f,0.7570f,0.0011f}, {1.0567f,0.6949f,0.0010f}, {1.0622f,0.6310f,0.0008f},
        {1.0456f,0.5668f,0.0006f}, {1.0026f,0.5030f,0.0003f}, {0.9384f,0.4412f,0.0002f},
        {0.8544f,0.3810f,0.0002f}, {0.7514f,0.3210f,0.0001f}, {0.6424f,0.2650f,0.0000f},
        {0.5419f,0.2170f,0.0000f}, {0.4479f,0.1750f,0.0000f}, {0.3608f,0.1382f,0.0000f},
        {0.2835f,0.1070f,0.0000f}, {0.2187f,0.0816f,0.0000f}, {0.1649f,0.0610f,0.0000f},
        {0.1212f,0.0446f,0.0000f}, {0.0874f,0.0320f,0.0000f}, {0.0636f,0.0232f,0.0000f},
        {0.0468f,0.0170f,0.0000f}, {0.0329f,0.0119f,0.0000f}, {0.0227f,0.0082f,0.0000f},
        {0.0158f,0.0057f,0.0000f}, {0.0114f,0.0041f,0.0000f}, {0.0081f,0.0029f,0.0000f},
        {0.0058f,0.0021f,0.0000f}, {0.0041f,0.0015f,0.0000f}, {0.0029f,0.0010f,0.0000f},
        {0.0020f,0.0007f,0.0000f}, {0.0014f,0.0005f,0.0000f}, {0.0010f,0.0004f,0.0000f},
        {0.0007f,0.0002f,0.0000f}, {0.0005f,0.0002f,0.0000f}, {0.0003f,0.0001f,0.0000f},
        {0.0002f,0.0001f,0.0000f}, {0.0002f,0.0001f,0.0000f}, {0.0001f,0.0000f,0.0000f},
        {0.0001f,0.0000f,0.0000f}, {0.0001f,0.0000f,0.0000f}, {0.0000f,0.0000f,0.0000f}
    };

    vec3 xyz = {0};
    for (i32 i = 0, lambda = 380; i < 81; i++, lambda += 5) {
        f32 intensity = glx_bb_spectrum(lambda, temp);
        xyz.x += intensity * cie_color_match[i][0];
        xyz.y += intensity * cie_color_match[i][1];
        xyz.z += intensity * cie_color_match[i][2];
    }

    f32 total = xyz.x + xyz.y + xyz.z;
    xyz.x /= total;
    xyz.y /= total;
    xyz.z /= total;

    return xyz;
}

internal vec3 glx_xyz_to_rgb(vec3 xyz) {
    local_persist const mat3 srgb = {{
     3.2406f, -1.5372f, -0.4986f,
    -0.9689f,  1.8758f,  0.0415f,
     0.0557f, -0.2040f,  1.0570f
    }};

    return mat3_mul_vec3(srgb, xyz);
}

internal vec3 glx_bb_color(f32 temp) {
    vec3 xyz = glx_spectrum_to_xyz(temp);
    vec3 rgb = glx_xyz_to_rgb(xyz);
    
    // constrain
    f32 w = MIN(0, MIN(rgb.x, MIN(rgb.y, rgb.z)));
    if (w < 0) {
        rgb.x -= w;
        rgb.y -= w;
        rgb.z -= w;
    }

    // normalize
    f32 m = MAX(rgb.x, MAX(rgb.y, rgb.z));
    if (m > 0) {
        rgb.x /= m;
        rgb.y /= m;
        rgb.z /= m;
    }

    return rgb;
}

internal void glx_ct_build(void) {
    f32 step = (TEMP_MAX - TEMP_MIN) / COLOR_TABLE_SIZE;
    for (i32 i = 0; i < COLOR_TABLE_SIZE; i++) {
        f32 temp = TEMP_MIN + i * step;
        vec3 rgb = glx_bb_color(temp);
        color_table[i] = rgb;
    }
}

internal vec3 glx_ct_lookup(f32 temp) {
    f32 step = (TEMP_MAX - TEMP_MIN) / COLOR_TABLE_SIZE;
    i32 i = (i32)((temp - TEMP_MIN) / step);
    i = MAX(0, MIN(COLOR_TABLE_SIZE - 1, i)); // clamp

    return color_table[i];
}

internal f32 glx_mass_disc(f32 rad_core, f32 r) {
    f32 disc_falloff = rad_core / 3.0f;
    f32 disc_thickness = 2000.0f;
    f32 disc_density_center = 1.0f;

    return disc_density_center * expf(-r / disc_falloff) * (r * r) * PI * disc_thickness;
}

internal f32 glx_mass_halo(f32 rad_glx, f32 r) {
    f32 halo_falloff = rad_glx / 6.0f;
    f32 halo_density_center = 0.15f;
    f32 x = r / halo_falloff;

    return halo_density_center * (1.0f / (1.0f + x * x)) * (4.0f * PI * r * r * r / 3.0f);
}

internal f32 glx_velocity_kms(f32 rad_core, f32 rad_galaxy, f32 r) {
    f32 mass_buffer = 100.0f;
    return 20000.0f * sqrtf(G * (glx_mass_halo(rad_galaxy, r) + glx_mass_disc(rad_core, r) + mass_buffer) / r) ;
}

internal f32 glx_orbital_velocity(f32 rad_core, f32 rad_galaxy, f32 r) {
    f32 v = glx_velocity_kms(rad_core, rad_galaxy, r);
    f32 circ = 2.0f * PI * r * PC_TO_KM;
    f32 time = circ / (v * SEC_PER_YEAR);

    return (360.0f / time) * (PI / 180.0f);
}

internal void glx_advance(glx_params *params, f32 dt) {
    params->elapsed_years += dt * params->time_scale;
}

internal void glx_init(glx_params *params, glx_vertex *vertices, u32 seed) {
    f32 radii[CDF_POINTS], cumulative[CDF_POINTS], inverse[CDF_POINTS];
    glx_cdf_build(radii, cumulative, inverse, params->rad_core, params->rad_galaxy);

    srand(seed);
    f32 rand_scale = 1.0f / RAND_MAX;
    for (i32 i = 0; i < params->num_stars; i++) {
        f32 radius = glx_cdf_sample(inverse, rand() * rand_scale);
        f32 theta0 = (rand() * rand_scale) * 2.0f * PI;
        f32 ang_speed = glx_orbital_velocity(params->rad_core, params->rad_galaxy, radius);

        f32 temp = 6000 + (4000 * rand() * rand_scale - 2000);
        vec3 color = glx_ct_lookup(temp);

        vertices[i].color.x = color.x;
        vertices[i].color.y = color.y;
        vertices[i].color.z = color.z;
        vertices[i].radius = radius;
        vertices[i].theta = theta0;
        vertices[i].ang_speed = ang_speed;
        vertices[i].mag = 0.1f + 0.4f * (rand() * rand_scale);
        vertices[i].kind = 0.0f;
    }

    srand(seed + 1);
    for (i32 i = 0; i < params->num_dust; i++) {
        i32 idx = params->num_stars + i;
        f32 radius;
        if (i % 2 == 0)
            radius = glx_cdf_sample(inverse, rand() * rand_scale);
        else {
            f32 x = 2 * params->rad_galaxy * (rand() * rand_scale) - params->rad_galaxy;
            f32 y = 2 * params->rad_galaxy * (rand() * rand_scale) - params->rad_galaxy;
            radius = sqrtf(x * x + y * y);
        }

        f32 theta = (rand() * rand_scale) * 2.0f * PI;
        f32 b = radius * glx_ex(params, radius);
        f32 avg_radius = (radius + b) / 2.0f;
        f32 ang_speed = glx_orbital_velocity(params->rad_core, params->rad_galaxy, avg_radius);

        f32 temp = 4000 + radius / 4.0f;
        vec3 color = glx_ct_lookup(temp);

        vertices[idx].color.x = color.x;
        vertices[idx].color.y = color.y;
        vertices[idx].color.z = color.z;
        vertices[idx].radius = radius;
        vertices[idx].theta = theta;
        vertices[idx].ang_speed = ang_speed;
        vertices[idx].mag = 0.02f + 0.15f * (rand() * rand_scale);
        vertices[idx].kind = 1.0f;
    }

    srand(seed + 2);
    i32 idx = params->num_stars + params->num_dust;
    for (i32 i = 0; i < params->num_dust / 100; i++) {
        f32 x = 2 * params->rad_galaxy * (rand() * rand_scale) - params->rad_galaxy;
        f32 y = 2 * params->rad_galaxy * (rand() * rand_scale) - params->rad_galaxy;
        f32 origin = sqrtf(x * x + y * y);

        f32 origin_theta =  (rand() * rand_scale) * 2.0f * PI;
        f32 mag = 0.1f + 0.05f * (rand() * rand_scale);
        
        i32 n = (i32)(100 * (rand() * rand_scale));
        for (i32 j = 0; j < n; j++) {
            f32 radius = origin + 200 - 400 * (rand() * rand_scale);
            f32 theta = origin_theta + (10.0f - 20.0f * (rand() * rand_scale)) * (PI / 180.0f);
            f32 b = radius * glx_ex(params, radius);
            f32 avg_radius = (radius + b) / 2.0f;
            f32 ang_speed = glx_orbital_velocity(params->rad_core, params->rad_galaxy, avg_radius);

            f32 temp = 4000 + radius / 4.5f - 1000;
            vec3 color = glx_ct_lookup(temp);

            vertices[idx].color.x = color.x;
            vertices[idx].color.y = color.y;
            vertices[idx].color.z = color.z;
            vertices[idx].radius = radius;
            vertices[idx].theta = theta;
            vertices[idx].ang_speed = ang_speed;
            vertices[idx].mag = mag + 0.025f * (rand() * rand_scale);
            vertices[idx].kind = 2.0f;
            idx++;
        }
    }

    params->num_particles = idx;
}
