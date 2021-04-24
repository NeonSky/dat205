#include "common.cuh"

// The current ray and its payload.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay , );
rtDeclareVariable(RayPayload, payload, rtPayload, );

// Environment map texture sampler.
rtTextureSampler<float4, 2> env_map;

RT_PROGRAM void miss_environment_constant() {

  // Azimuth; angle from the ray's z-axis (ccw) to (x, z). Reference: https://www.wikiwand.com/en/Spherical_coordinate_system
  float theta = atan2f(ray.direction.x, ray.direction.z);

  // Altitude; angle from y-axis (down) to (x, y, z).
  float phi = 0.5f * M_PIf - acosf(ray.direction.y); // NOTE: no division by ray length, since it is normalized.

  // Derive texture coordinates. NOTE: M_1_PIf = 1 / M_PIf
  float u = 0.5f * theta * M_1_PIf;   // "theta" is in [0.0, 2pi], hence "0.5 * theta / pi" is in [0.0, 1.0]
  float v = 0.5f * (1.0f + sin(phi)); // "sin(phi)" is in [-1.0, 1.0], hence "0.5 + 0.5 * sin(phi)" is in [0.0, 1.0]

  // Artificially brighten the environment map a bit.
  float3 ambient_term = make_float3(0.1f, 0.1f, 0.1f);

  payload.radiance = make_float3(tex2D(env_map, u, v)) + ambient_term;
}
