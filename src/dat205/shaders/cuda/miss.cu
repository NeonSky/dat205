#include "common.cuh"

// The payload/context of the current ray.
rtDeclareVariable(RayPayload, payload, rtPayload, );

RT_PROGRAM void miss_environment_constant() {
  payload.radiance = make_float3(0.0f, 0.0f, 0.0f);
}
