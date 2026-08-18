#ifndef GALAXY_SIM_H
#define GALAXY_SIM_H

#define I0_BULGE   1.0f
#define K          0.02f

#define CDF_STEPS  1000
#define CDF_POINTS (CDF_STEPS / 2)

#define PLANCK_C1 3.74183e-16f
#define PLANCK_C2 1.4388e-2f

#define COLOR_TABLE_SIZE 1000
#define TEMP_MIN         1000.0f
#define TEMP_MAX         12000.0f

#define G             6.672e-11f
#define PC_TO_KM      3.08567758129e13f
#define SEC_PER_YEAR  (365.25f * 86400.0f)

internal f32 glx_ex(const glx_params *glx, f32 radius);
internal f32 glx_brightness(f32 rad_core, f32 rad_galaxy, f32 radius);
internal void glx_cdf_build(f32 *radii, f32 *cumulative, f32 *inverse, f32 rad_core, f32 rad_galaxy);
internal f32 glx_cdf_sample(const f32 *inverse, f32 p);
internal f32 glx_bb_spectrum(i32 wavelength, f32 temp);
internal vec3 glx_spectrum_to_xyz(f32 temp);
internal vec3 glx_xyz_to_rgb(vec3 xyz);
internal vec3 glx_bb_color(f32 temp);
internal void glx_ct_build(void);
internal vec3 glx_ct_lookup(f32 temp);
internal f32 glx_mass_disc(f32 rad_core, f32 r);
internal f32 glx_mass_halo(f32 rad_galaxy, f32 r);
internal f32 glx_velocity_kms(f32 rad_core, f32 rad_galaxy, f32 r);
internal f32 glx_orbital_velocity(f32 rad_core, f32 rad_galaxy, f32 r);
internal void glx_init(glx_params *glx, glx_vertex *vertices, u32 seed);

#endif
