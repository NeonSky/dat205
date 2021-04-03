#include "common.cuh"

// The current ray and its payload.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay , );
rtDeclareVariable(ShadowRayPayload, payload, rtPayload, );

// Properties of the hit surface's material.
rtDeclareVariable(float, mat_transparency, , );

// Attributes from intersection test.
rtDeclareVariable(optix::float3, attr_normal, attribute NORMAL, );

RT_PROGRAM void any_hit() {
  float3 normal = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_normal));
  float n_dot_r = fabs(optix::dot(normal, ray.direction));

  float attenuation = 1.0f - mat_transparency;
  float F = attenuation + (1.0f - attenuation) * pow(1.0f - n_dot_r, 5.0f);

  payload.attenuation *= 1.0f - F;

  rtIgnoreIntersection();
}
