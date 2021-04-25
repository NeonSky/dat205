#include "common.cuh"

// The current ray and its payload.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay , );
rtDeclareVariable(ShadowRayPayload, payload, rtPayload, );

// Properties of the hit surface's material.
rtDeclareVariable(float, mat_transparency, , );

// Attributes from intersection test.
rtDeclareVariable(optix::float3, attr_normal, attribute NORMAL, );

RT_PROGRAM void any_hit() {
  if (mat_transparency <= 0.0f) {
    payload.attenuation = make_float3(0.0f);
    rtTerminateRay();
  }

  // Reflect/absorb based on fresnel.
  float3 normal = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_normal));
  float n_dot_i = fabs(optix::dot(normal, ray.direction));

  float attenuation = 1.0f - mat_transparency;
  float F = attenuation + (1.0f - attenuation) * pow(1.0f - n_dot_i, 5.0f);

  payload.attenuation *= 1.0f - F;

  rtIgnoreIntersection();
}
